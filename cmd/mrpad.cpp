/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "image.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;



void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Pad an image to increase the FOV";

  ARGUMENTS
  + Argument ("image_in",
              "the image to be padded").type_image_in()
  + Argument ("image_out",
              "the output path for the resulting padded image").type_image_out();

  OPTIONS
  + Option   ("uniform",
              "pad the input image by a uniform number of voxels on all sides (in 3D)")
  + Argument ("number").type_integer (0)

  + Option   ("axis",
              "pad the input image along the provided axis (defined by index). Lower and upper define "
              "the number of voxels to add to the lower and upper bounds of the axis").allow_multiple()
  + Argument ("index").type_integer (0, 2)
  + Argument ("lower").type_integer (0)
  + Argument ("upper").type_integer (0);
}


void run ()
{
  Header input_header = Header::open (argument[0]);
  auto input = input_header.get_image<float>();

  ssize_t bounds[3][2] = { {0, input_header.size (0) - 1},
                       {0, input_header.size (1) - 1},
                       {0, input_header.size (2) - 1} };

  ssize_t padding[3][2] = { {0, 0}, {0, 0}, {0, 0} };

  auto opt = get_options ("uniform");
  if (opt.size()) {
    ssize_t pad = opt[0][0];
    for (size_t axis = 0; axis < 3; axis++) {
      padding[axis][0] = pad;
      padding[axis][1] = pad;
    }
  }

  opt = get_options ("axis");
  for (size_t i = 0; i != opt.size(); ++i) {
    // Manual padding of axis overrides uniform padding
    const size_t axis  = opt[i][0];
    padding[axis][0] = opt[i][1];
    padding[axis][1] = opt[i][2];
  }

  Header output_header (input_header);
  auto output_transform = input_header.transform();
  for (size_t axis = 0; axis < 3; ++axis) {
    output_header.size (axis) = output_header.size(axis) + padding[axis][0] + padding[axis][1];
    output_transform (axis, 3) +=	(output_transform (axis, 0) * (bounds[0][0] - padding[0][0]) * input_header.spacing (0))
                                + (output_transform (axis, 1) * (bounds[1][0] - padding[0][0]) * input_header.spacing (1))
                                + (output_transform (axis, 2) * (bounds[2][0] - padding[0][0]) * input_header.spacing (2));
  }
  output_header.transform() = output_transform;
  auto output = Image<float>::create (argument[1], output_header);

  for (auto l = Loop ("padding image... ", output) (output); l; ++l) {
    bool in_bounds = true;
    for (size_t axis = 0; axis < 3; ++axis) {
      input.index(axis) = output.index(axis) - padding[axis][0];
      if (input.index(axis) < 0 || input.index(axis) >= input_header.size (axis))
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

