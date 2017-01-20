/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include <set>
#include <vector>

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "transform.h"
#include "algo/copy.h"

using namespace MR;
using namespace App;


// TODO:
// * Operate on mask images rather than arbitrary images?
// * Remove capability to edit in-place - just deal with image swapping in the script?
// * Tests


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "Directly edit the intensities within an image from the command-line. "

    "A range of options are provided to enable direct editing of "
    "voxel intensities based on voxel / real-space coordinates. "

    "If only one image path is provided, the image will be edited in-place "
    "(use at own risk); if input and output image paths are provided, the "
    "output will contain the edited image, and the original image will not "
    "be modified in any way.";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in()
  + Argument ("output", "the (optional) output image").type_image_out().optional();

  OPTIONS
  + Option ("plane", "fill one or more planes on a particular image axis").allow_multiple()
    + Argument ("axis").type_integer (0, 2)
    + Argument ("coord").type_sequence_int()
    + Argument ("value").type_float()

  + Option ("sphere", "draw a sphere with radius in mm").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("radius").type_float()
    + Argument ("value").type_float()

  + Option ("voxel", "change the image value within a single voxel").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("scanner", "indicate that coordinates are specified in scanner space, rather than as voxel coordinates");

}



class Vox : public Eigen::Array3i
{
  public:
    using Eigen::Array3i::Array3i;
    Vox (const Eigen::Vector3& p) :
        Eigen::Array3i { int(std::round (p[0])), int(std::round (p[1])), int(std::round (p[2])) } { }
    bool operator< (const Vox& i) const {
      return (i[0] == (*this)[0] ? (i[1] == (*this)[1] ? (i[2] < (*this)[2]) : (i[1] < (*this)[1])) : (i[0] < (*this)[0]));
    }
};



const Vox voxel_offsets[6] = { { 0,  0, -1},
                               { 0,  0,  1},
                               { 0, -1,  0},
                               { 0,  1,  0},
                               {-1,  0,  0},
                               { 1,  0,  0} };



void run ()
{
  bool inplace = (argument.size() == 1);
  auto H = Header::open (argument[0]);
  auto in = H.get_image<float> (inplace); // Need to set read/write flag
  Image<float> out;
  if (inplace) {
    out = Image<float> (in);
  } else {
    if (std::string(argument[1]) == std::string(argument[0])) // Not ideal test - could be different paths to the same file
      throw Exception ("Do not provide same image as input and output; instad specify image to be edited in-place");
    out = Image<float>::create (argument[1], H);
    copy (in, out);
  }

  Transform transform (H);
  const bool scanner = get_options ("scanner").size();
  if (scanner && H.ndim() < 3)
    throw Exception ("Cannot specify scanner-space coordinates if image has less than 3 dimensions");

  size_t operation_count = 0;

  auto opt = get_options ("plane");
  if (opt.size()) {
    if (H.ndim() != 3)
      throw Exception ("-plane option only works for 3D images");
    if (scanner)
      throw Exception ("-plane option cannot be used with scanner-space coordinates");
  }
  operation_count += opt.size();
  for (auto p : opt) {
    const size_t axis = p[0];
    const auto coords = parse_ints (p[1]);
    const float value = p[2];
    const std::array<size_t, 2> loop_axes ( { axis == 0 ? size_t(1) : size_t(0), axis == 2 ? size_t(1) : size_t(2) } );
    for (auto c : coords) {
      out.index (axis) = c;
      for (auto outer = Loop(loop_axes[0]) (out); outer; ++outer) {
        for (auto inner = Loop(loop_axes[1]) (out); inner; ++inner)
          out.value() = value;
      }
    }
  }

  opt = get_options ("sphere");
  if (opt.size() && H.ndim() != 3)
    throw Exception ("-sphere option only works for 3D images");
  operation_count += opt.size();
  for (auto s : opt) {
    const auto position = parse_floats (s[0]);
    Eigen::Vector3 centre_scannerspace (position[0], position[1], position[2]);
    const default_type radius = s[1];
    const float value = s[2];
    if (position.size() != 3)
      throw Exception ("Centre of sphere must be defined using 3 comma-separated values");
    Eigen::Vector3 centre_voxelspace (centre_scannerspace);
    if (scanner)
      centre_voxelspace = transform.scanner2voxel * centre_scannerspace;
    else
      centre_scannerspace = transform.voxel2scanner * centre_voxelspace;
    std::set<Vox> processed;
    std::vector<Vox> to_expand;
    const Vox seed_voxel (centre_voxelspace);
    processed.insert (seed_voxel);
    to_expand.push_back (seed_voxel);
    while (to_expand.size()) {
      const Vox v (to_expand.back());
      to_expand.pop_back();
      const Eigen::Vector3 v_scanner = transform.voxel2scanner * v.matrix().cast<default_type>();
      const default_type distance = (v_scanner - centre_scannerspace).norm();
      if (distance < radius) {
        if (!is_out_of_bounds (H, v)) {
          assign_pos_of (v).to (out);
          out.value() = value;
        }
        for (size_t i = 0; i != 6; ++i) {
          const Vox v_adj (v + voxel_offsets[i]);
          if (processed.find (v_adj) == processed.end()) {
            processed.insert (v_adj);
            to_expand.push_back (v_adj);
          }
        }
      }
    }
  }

  opt = get_options ("voxel");
  operation_count += opt.size();
  for (auto v : opt) {
    const auto position = parse_floats (v[0]);
    const float value = v[1];
    if (position.size() != H.ndim())
      throw Exception ("Image has " + str(H.ndim()) + " dimensions, but -voxel option position " + std::string(v[0]) + " provides only " + str(position.size()) + " coordinates");
    if (scanner) {
      Eigen::Vector3 p (position[0], position[1], position[2]);
      p = transform.scanner2voxel * p;
      const Vox voxel (p);
      assign_pos_of (voxel).to (out);
      for (size_t axis = 3; axis != out.ndim(); ++axis) {
        if (std::round (position[axis]) != position[axis])
          throw Exception ("Non-spatial coordinates provided using -voxel option must be provided as integers");
        out.index(axis) = position[axis];
      }
    } else {
      for (size_t axis = 0; axis != out.ndim(); ++axis) {
        if (std::round (position[axis]) != position[axis])
          throw Exception ("Voxel coordinates provided using -voxel option must be provided as integers");
        out.index(axis) = position[axis];
      }
    }
    out.value() = value;
  }

  if (!operation_count) {
    if (inplace) {
      WARN ("No edits specified; image will be unaffected");
    } else {
      WARN ("No edits specified; output image will be copy of input");
    }
  }
}
