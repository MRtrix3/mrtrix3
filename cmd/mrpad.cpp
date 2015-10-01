/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 2014

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "command.h"
#include "image.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;



void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Pad an image to increase the FOV";

  ARGUMENTS
  + Argument ("image_in",
              "the image to be padded").type_image_in()
  + Argument ("image_out",
              "the output path for the resulting padded image").type_image_out();

  OPTIONS
  + Option   ("uniform",
              "pad the input image by a uniform number of voxels on all sides (in 3D)")
  + Argument ("number").type_integer (0, 0, 1e6)

  + Option   ("axis",
              "pad the input image along the provided axis (defined by index). Lower and upper define "
              "the number of voxels to add to the lower and upper bounds of the axis").allow_multiple()
  + Argument ("index").type_integer (0, 0, 2)
  + Argument ("lower").type_integer (0, 0, 1e6)
  + Argument ("upper").type_integer (0, 1e6, 1e6);
}


void run ()
{
  Header input_header = Header::open (argument[0]);
  auto input = input_header.get_image<float>();

  int bounds[3][2] = { {0, input_header.size (0) - 1},
                       {0, input_header.size (1) - 1},
                       {0, input_header.size (2) - 1} };

  int padding[3][2] = { {0, 0}, {0, 0}, {0, 0} };

  auto opt = get_options ("uniform");
  if (opt.size()) {
    int pad = opt[0][0];
    for (int axis = 0; axis < 3; axis++) {
      padding[axis][0] = pad;
      padding[axis][1] = pad;
    }
  }

  opt = get_options ("axis");
  for (size_t i = 0; i != opt.size(); ++i) {
    // Manual padding of axis overrides uniform padding
    const int axis  = opt[i][0];
    padding[axis][0] = opt[i][1];
    padding[axis][1] = opt[i][2];
  }

  Header output_header (input_header);
  auto output_transform = input_header.transform();
  for (int axis = 0; axis < 3; ++axis) {
    output_header.dim (axis) = output_header.dim(axis) + padding[axis][0] + padding[axis][1];
    output_transform (axis, 3) +=	(output_transform (axis, 0) * (bounds[0][0] - padding[0][0]) * input_header.vox (0))
                                + (output_transform (axis, 1) * (bounds[1][0] - padding[0][0]) * input_header.vox (1))
                                + (output_transform (axis, 2) * (bounds[2][0] - padding[0][0]) * input_header.vox (2));
  }
  output_header.transform() = output_transform;
  auto output = Image<float>::create (argument[1], output_header);

  for (auto l = Loop ("padding image... ", output) (output); l; ++l) {
    bool in_bounds = true;
    for (int axis = 0; axis < 3; ++axis) {
      input[axis] = output[axis] - padding[axis][0];
      if (input[axis] < 0 || input[axis] >= input_header.size (axis))
        in_bounds = false;
    }
    if (input.ndim() > 3)
      input.index (3) = output.index (3);
    if (in_bounds)
      output.value() = input.value();
    else
      output.value() = 0;
  }

}

