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


#include "app.h"
#include "image/loop.h"
#include "image/copy.h"
#include "image/adapter/subset.h"
#include "image/buffer.h"
#include "image/voxel.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
    + "Crop an image series to a reduced field of view, using either manual setting of axis dimensions, or a computed mask image corresponding to the brain. \n"
    + "If using a mask, a gap of 1 voxel will be left at all 6 edges of the image such that trilinear interpolation upon the resulting images is still valid. \n"
    + "This is useful for axially-acquired brain images, where the image size can be reduced by a factor of 2 by removing the empty space on either side of the brain.";

  ARGUMENTS
    + Argument ("image_in",  "the image to be cropped").type_image_in()
    + Argument ("image_out", "the output path for the resulting cropped image").type_image_out();


  OPTIONS

    + Option   ("mask",  "crop the input image according to the spatial extent of a mask image")
    + Argument ("image", "the mask image").type_image_in()

    + Option   ("axis",  "crop the input image in the provided axis").allow_multiple()
    + Argument ("index", "the index of the image axis to be cropped").type_integer (0, 0, 2)
    + Argument ("start", "the first voxel along this axis to be included in the output image").type_integer (0, 0, 1e6)
    + Argument ("end",   "the last voxel along this axis to be included in the output image").type_integer (0, 1e6, 1e6);

}




void run ()
{

  Image::Buffer<float> data_in (argument[0]);
  Image::Buffer<float>::voxel_type voxel_in (data_in);

  int bounds[3][2] = { {0, data_in.dim (0) - 1}, {0, data_in.dim (1) - 1}, {0, data_in.dim (2) - 1} };

  Options opt = get_options ("mask");
  if (opt.size()) {

    Image::Buffer<bool> data_mask (opt[0][0]);
    Image::check_dimensions (data_in, data_mask, 0, 3);
    Image::Buffer<bool>::voxel_type voxel_mask (data_mask);

    for (int axis = 0; axis != 3; ++axis) {
      bounds[axis][0] = data_in.dim (axis);
      bounds[axis][1] = 0;
    }

    Image::Loop loop_mask;
    for (loop_mask.start (voxel_mask); loop_mask.ok(); loop_mask.next (voxel_mask)) {
      if (voxel_mask.value()) {
        for (int axis = 0; axis != 3; ++axis) {
          bounds[axis][0] = std::min (bounds[axis][0], int (voxel_mask[axis]));
          bounds[axis][1] = std::max (bounds[axis][1], int (voxel_mask[axis]));
        }
      }
    }

    for (int axis = 0; axis != 3; ++axis) {
      if (bounds[axis][0] > bounds[axis][1])
        throw Exception ("mask image is empty; can't use to crop image");
      if (bounds[axis][0])
        --bounds[axis][0];
      if (bounds[axis][1] < voxel_mask.dim (axis) - 1)
        ++bounds[axis][1];
    }

  }

  opt = get_options ("axis");
  for (size_t i = 0; i != opt.size(); ++i) {
    // Manual cropping of axis overrides mask image bounds
    const int axis  = opt[i][0];
    const int start = opt[i][1];
    const int end   = opt[i][2];
    bounds[axis][0] = start;
    bounds[axis][1] = end;
    if (bounds[axis][0] < 0 || bounds[axis][1] >= data_in.dim(axis))
      throw Exception ("Index supplied for axis " + str(axis) + " is out of bounds.");
  }


  size_t from[] = { bounds[0][0], bounds[1][0], bounds[2][0] };
  size_t dim[] = { bounds[0][1]-from[0]+1, bounds[1][1]-from[1]+1, bounds[2][1]-from[2]+1 };

  Image::Adapter::Subset<Image::Buffer<float>::voxel_type> cropped (voxel_in, from, dim);

  Image::Header H_out (data_in);
  H_out.info() = cropped.info();
  Image::Buffer<float> data_out (argument[1], H_out);
  Image::Buffer<float>::voxel_type voxel_out (data_out);
  Image::copy_with_progress_message ("cropping image...", cropped, voxel_out);

}

