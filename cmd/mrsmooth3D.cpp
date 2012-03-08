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
#include "image/voxel.h"
#include "image/filter/gaussian3D.h"
#include "progressbar.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "smooth images by a convolution with a Gaussian kernel. "
    "If the input is a 4D image, each 3D volume is smoothed independently.";

  ARGUMENTS
  + Argument ("input", "input image to be smoothed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("stdev", "apply Gaussian smoothing with the specified standard deviation."
            "The standard deviation is defined in mm (Default 1mm)."
            "This can be specified either as a single value to be used for all 3 axes, "
            "or as a comma-separated list of 3 values, one for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all 3 axes, "
            "or as a comma-separated list of 3 values, one for each axis."
            "The default extent is 4 standard deviations.")
  + Argument ("size").type_sequence_int();
}


void run () {

  std::vector<float> stdev (1);
  stdev[0] = 1.0;
  Options opt = get_options ("stdev");
  if (opt.size()) {
    stdev = parse_floats (opt[0][0]);
    for (size_t i = 0; i < stdev.size(); ++i)
      if (stdev[i] < 0.0)
        throw Exception ("the Gaussian stdev values cannot be negative");
    if (stdev.size() != 1 && stdev.size() != 3)
      throw Exception ("unexpected number of elements specified in Gaussian stdev");
  }

  std::vector<int> extent (1);
  extent[0] = 0;
  opt = get_options ("extent");
  if (opt.size()) {
    extent = parse_ints (opt[0][0]);
    if (extent.size() != 1 && extent.size() != 3)
      throw Exception ("unexpected number of elements specified in extent");
    for (size_t i = 0; i < extent.size(); ++i) {
      if (! (extent[i] & int (1)))
        throw Exception ("expected odd number for extent");
      if (extent[i] < 0)
        throw Exception ("the kernel extent must be positive");
    }
  }

  Image::BufferPreload<float> src_data (argument[0]);
  Image::BufferPreload<float>::voxel_type src (src_data);
  Image::Filter::Gaussian3D smooth_filter (src, extent, stdev);

  Image::Header header (src_data);
  header.info() = smooth_filter.info();
  header.datatype() = src_data.datatype();

  Image::Buffer<float> dest_data (argument[1], header);
  Image::Buffer<float>::voxel_type dest (dest_data);

  ProgressBar progress("smoothing image...");
  smooth_filter (src, dest);
}
