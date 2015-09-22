/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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
#include "image/loop.h"
#include "image/buffer.h"
#include "image/voxel.h"

using namespace MR;
using namespace App;



void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Pad an image to increase the FOV";

  ARGUMENTS
  + Argument ("image_in",
              "the image to be padded").type_image_in()
  + Argument ("image_out",
              "the output path for the resulting padded image").type_image_out();

  OPTIONS
  + Option   ("uniform",
              "pad the input image by a uniform amount on all sides")
  + Argument ("number",
              "the number of voxels to pad").type_integer (0, 0, 1e6)

  + Option   ("axis",
              "pad the input image in the provided axis").allow_multiple()
  + Argument ("index",
              "the index of the image axis to be padded").type_integer (0, 0, 2)
  + Argument ("lower",
              "the number of voxels to pad at the lower bound "
              "of this axis").type_integer (0, 0, 1e6)
  + Argument ("upper",
              "the number of voxels to pad at the upper bound "
              "of this axis").type_integer (0, 1e6, 1e6);
}


void run ()
{

  Image::Header input_header (argument[0]);
  Image::Buffer<float> input_data (input_header);
  Image::Buffer<float>::voxel_type input_voxel (input_data);

  int bounds[3][2] = { {0, input_header.dim (0) - 1},
                       {0, input_header.dim (1) - 1},
                       {0, input_header.dim (2) - 1} };

  int padding[3][2] = { {0 , 0}, {0, 0}, {0, 0} };

  Options opt = get_options ("uniform");
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

  Image::Header output_header (input_header);
  Math::Matrix<float> output_transform (input_header.transform());
  for (int axis = 0; axis < 3; ++axis) {
    output_header.dim (axis) = output_header.dim(axis) + padding[axis][0] + padding[axis][1];
    output_transform (axis, 3) +=	(output_transform (axis, 0) * (bounds[0][0] - padding[0][0]) * input_header.vox (0))
                                + (output_transform (axis, 1) * (bounds[1][0] - padding[0][0]) * input_header.vox (1))
                                + (output_transform (axis, 2) * (bounds[2][0] - padding[0][0]) * input_header.vox (2));
  }
  output_header.transform() = output_transform;
  Image::Buffer<float> output_data (argument[1], output_header);
  auto output_voxel (output_data);

  Image::Loop loop ("padding image...");
  for (loop.start (output_voxel); loop.ok(); loop.next (output_voxel)) {
    bool in_bounds = true;
    for (int axis = 0; axis < 3; ++axis) {
      input_voxel[axis] = output_voxel[axis] - padding[axis][0];
      if (input_voxel[axis] < 0 || input_voxel[axis] >= input_header.dim (axis))
        in_bounds = false;
    }
    if (input_voxel.ndim() > 3)
      input_voxel[3] = output_voxel[3];
    if (in_bounds)
      output_voxel.value() = input_voxel.value();
    else
      output_voxel.value() = 0;
  }

}

