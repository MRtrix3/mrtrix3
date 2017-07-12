/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "mrtrix.h"
#include "transform.h"

#include "algo/loop.h"
#include "file/path.h"
#include "file/utils.h"
#include "interp/nearest.h"

#include "connectome/connectome.h"
#include "connectome/lut.h"

#include <string>


#define SPINE_NODE_NAME std::string("Spinal_column")




using namespace MR;
using namespace App;
using namespace MR::Connectome;
using MR::Connectome::node_t;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert a connectome node image from one lookup table to another";

  DESCRIPTION
  + "Typical usage is to convert a parcellation image provided by some other software, based on "
    "the lookup table provided by that software, to conform to a new lookup table, particularly "
    "one where the node indices increment from 1, in preparation for connectome construction; "
    "examples of such target lookup table files are provided in share//mrtrix3//labelconvert//";


  ARGUMENTS
  + Argument ("path_in",   "the input image").type_image_in()
  + Argument ("lut_in",    "the connectome lookup table for the input image").type_file_in()
  + Argument ("lut_out",   "the target connectome lookup table for the output image").type_file_in()
  + Argument ("image_out", "the output image").type_image_out();


  OPTIONS
  + Option ("spine", "provide a manually-defined segmentation of the base of the spine where the streamlines terminate, so that "
                     "this can become a node in the connection matrix.")
    + Argument ("image").type_image_in();

}



void run ()
{

  // Open the input file
  auto H = Header::open (argument[0]);
  Connectome::check (H);
  auto in = H.get_image<node_t>();

  // Load the lookup tables
  LUT lut_in (argument[1]), lut_out (argument[2]);

  // Build the mapping from input to output indices
  const auto mapping = get_lut_mapping (lut_in, lut_out);

  // Modify the header for the output file
  H.datatype() = DataType::from<node_t>();
  add_line (H.keyval()["comments"], "LUT: " + Path::basename (argument[2]));

  // Create the output file
  auto out = Image<node_t>::create (argument[3], H);

  // Fill the output image with data
  bool user_warn = false;
  for (auto l = Loop (in) (in, out); l; ++l) {
    const node_t orig = in.value();
    if (orig < mapping.size())
      out.value() = mapping[orig];
    else
      user_warn = true;
  }
  if (user_warn)
    WARN ("Unexpected values detected in input image; suggest checking input image thoroughly");

  // Need to manually search through the output LUT to see if the
  //   'Spinal_column' node is in there, and appears only once
  node_t spine_index = 0;
  bool duplicates = false;
  for (const auto& i : lut_out) {
    if (i.second.get_name() == SPINE_NODE_NAME) {
      if (!spine_index)
        spine_index = i.first;
      else
        duplicates = true;
    }
  }

  // If the spine segment option has been provided, add this retrospectively
  auto opt = get_options ("spine");
  if (opt.size()) {

    if (duplicates) {
      WARN ("Could not add spine node: \"" + SPINE_NODE_NAME + "\" appears multiple times in output LUT");
    } else {

      auto in_spine = Image<bool>::open (opt[0][0]);
      if (dimensions_match (in_spine, out)) {

        for (auto l = Loop (in_spine) (in_spine, out); l; ++l) {
          if (in_spine.value())
            out.value() = spine_index;
        }

      } else {

        WARN ("Spine node is being created from the mask image provided using -spine option using nearest-neighbour interpolation;");
        WARN ("recommend using the parcellation image as the basis for this mask so that interpolation is not required");

        Transform transform (out);
        Interp::Nearest<decltype(in_spine)> nearest (in_spine);
        for (auto l = Loop (out) (out); l; ++l) {
          Eigen::Vector3 p (out.index (0), out.index (1), out.index (2));
          p = transform.voxel2scanner * p;
          if (nearest.scanner (p) && nearest.value())
            out.value() = spine_index;
        }

      }

    }

  } else if (spine_index) {
    WARN ("Config file includes \"" + SPINE_NODE_NAME + "\" node, but user has not provided the segmentation using -spine option");
  }

}

