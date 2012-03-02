/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 17/02/2012.

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
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/filter/gaussian3D.h"
#include "image/filter/gradient3D.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "compute the image gradient along the x, y, and z axes of a 3D image.";

  ARGUMENTS
  + Argument ("input", "input 3D image.").type_image_in ()
  + Argument ("output", "the output 4D gradient image.").type_image_out ();

  OPTIONS
  + Option ("stdev", "the standard deviation of the Gaussian kernel used to "
            "smooth the input image (in mm). The image is smoothed to reduced large "
            "spurious gradients caused by noise. Use this option to override "
            "the default stdev of 1 voxel. This can be specified either as a single "
            "value to be used for all 3 axes, or as a comma-separated list of "
            "3 values, one for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("scanner", "compute the gradient with respect to the scanner coordinate "
            "frame of reference.");
}


void run () {


  Image::BufferPreload<float> input_data (argument[0]);
  Image::BufferPreload<float>::voxel_type input_voxel (input_data);

  if (input_data.ndim() != 3)
    throw Exception("input image must be 3D");

  Image::Filter::Gaussian3D smooth_filter (input_voxel);

  Options opt = get_options ("stdev");
  if (opt.size()) {
  std::vector<float> stdev = parse_floats (opt[0][0]);
    for (size_t i = 0; i < stdev.size(); ++i)
      if (stdev[i] < 0.0)
        throw Exception ("the Gaussian stdev values cannot be negative");
    if (stdev.size() != 1 && stdev.size() != 3)
      throw Exception ("unexpected number of elements specified in Gaussian stdev");
    smooth_filter.set_stdev(stdev);
  }

  Image::Filter::Gradient3D gradient_filter (input_voxel);

  opt = get_options("scanner");
  if(opt.size()) {
    gradient_filter.compute_wrt_scanner(true);
  }

  Image::Header smooth_header (input_data);
  smooth_header.info() = smooth_filter.info();

  Image::BufferScratch<float> smoothed_data (smooth_header);
  Image::BufferScratch<float>::voxel_type smoothed_voxel (smoothed_data);

  Image::Header gradient_header (input_data);
  gradient_header.info() = gradient_filter.info();

  Image::Buffer<float> gradient_data (argument[1], gradient_header);
  Image::Buffer<float>::voxel_type gradient_voxel (gradient_data);

  smooth_filter (input_voxel, smoothed_voxel);
  gradient_filter (smoothed_voxel, gradient_voxel);
}
