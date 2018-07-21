/* Copyright (c) 2008-2018 the MRtrix3 contributors.
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
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "transform.h"
#include "types.h"

#include "connectome/connectome.h"

using namespace MR;
using namespace App;


const char * field_choices[] = { "mass", "centre", nullptr };


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compute statistics of parcels within a label image";

  ARGUMENTS
  + Argument ("input", "the input label image").type_image_in();

  OPTIONS
  + Option ("output", "output only the field specified; "
                      "options are: " + join(field_choices, ","))
    + Argument ("choice").type_choice (field_choices)

  + Option ("voxelspace", "report parcel centres of mass in voxel space rather than scanner space");

}


using Connectome::node_t;
using vector_type = Eigen::Array<default_type, Eigen::Dynamic, 1>;
using matrix_type = Eigen::Matrix<default_type, Eigen::Dynamic, 3>;



void run ()
{
  Header H = Header::open (argument[0]);
  if (H.ndim() > 3)
    throw Exception ("Command does not accept images with more than 3 dimensions");
  Connectome::check (H);
  Image<node_t> image = H.get_image<node_t>();

  matrix_type coms;
  vector_type masses;

  for (auto l = Loop(image) (image); l; ++l) {
    const node_t value = image.value();
    if (value) {
      if (value > coms.rows()) {
        coms.conservativeResizeLike (matrix_type::Zero (value, 3));
        masses.conservativeResizeLike (vector_type::Zero (value));
      }
      coms.row(value-1) += Eigen::Vector3 (image.index(0), image.index(1), image.index(2));
      masses[value-1]++;
    }
  }

  coms = coms.array().colwise() / masses;

  if (!get_options("voxelspace").size())
    coms = (image.transform() * coms.transpose()).transpose();

  auto opt = get_options ("output");
  if (opt.size()) {
    switch (int(opt[0][0])) {
      case 0: std::cout << masses; break;
      case 1: std::cout << coms; break;
      default: assert (0);
    }
    return;
  }

  // Ensure that the printout of centres-of-mass is nicely formatted
  Eigen::IOFormat fmt (Eigen::StreamPrecision, 0, ", ", "\n", "[ ", " ]", "", "");
  std::stringstream com_stringstream;
  com_stringstream << coms.format (fmt);
  const vector<std::string> com_strings = split(com_stringstream.str(), "\n");
  assert (com_strings.size() == masses.size());

  // Find width of first non-empty string, in order to centralise header label
  size_t com_width = 0;
  for (const auto& i : com_strings) {
    if (i.size()) {
      com_width = i.size();
      break;
    }
  }

  std::cout << std::setw(8) << std::right << "index" << " "
            << std::setw(8) << std::right << "mass" << " "
            << std::setw(4) << std::right << " "
            << std::setw(com_width/2 + std::string("centre of mass").size()/2) << std::right << "centre of mass" << "\n";
  for (ssize_t i = 0; i != masses.size(); ++i) {
    if (masses[i]) {
      std::cout << std::setw(8) << std::right << i+1 << " "
                << std::setw(8) << std::right << masses[i] << " "
                << std::setw(4+com_width) << std::right << com_strings[i] << "\n";
    }
  }
}
