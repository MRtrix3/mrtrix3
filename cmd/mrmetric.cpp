/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 2013

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
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
    + "compute the metric distance between two input images. Currently sum of squares only";

  ARGUMENTS
    + Argument ("input", "image 1").type_image_in ()
    + Argument ("input", "image 2").type_image_in ();

}

typedef float value_type;


void run ()
{
  Image::Buffer<float> input1 (argument[0]);
  Image::Buffer<float>::voxel_type input1_vox (input1);
  Image::Buffer<float> input2 (argument[1]);
  Image::Buffer<float>::voxel_type input2_vox (input2);

  Image::check_dimensions (input1_vox, input2_vox);
  Image::LoopInOrder loop (input1_vox);

  double total = 0.0;
  for (loop.start (input1_vox, input2_vox); loop.ok(); loop.next (input1_vox, input2_vox))
    total += Math::pow2(input1_vox.value() - input2_vox.value());
  CONSOLE ("sum of squares: " + str(total));

}
