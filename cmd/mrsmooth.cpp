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
#include "image/filter/gaussian_smooth.h"
#include "progressbar.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "smooth n-dimensional images by a convolution with a Gaussian kernel. "
  + "Note that if the input volume is 4D, then by default each 3D volume is smoothed independently. However "
    "this behaviour can be overridden by manually setting the stdev for all dimensions.";

  ARGUMENTS
  + Argument ("input", "input image to be smoothed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("stdev", "apply Gaussian smoothing with the specified standard deviation. "
            "The standard deviation is defined in mm (Default 1mm). "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the stdev for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the extent for each axis. "
            "The default extent is 4 standard deviations.")
  + Argument ("size").type_sequence_int()

  + Option ("anisotropic", "smooth each 3D volume of a 4D image using an anisotropic gaussian kernel "
                           "oriented along a corresponding direction. Note that when this option is used "
                           "the -stdev option defines the standard deviation along each eigenvector of the kernel."
                           "This can be used to smooth AFD values sampled along a number of directions. "
                           "The directions can be specified within the header of the input image, or by using the -directions option.")

  + Option ("directions", "the directions for anisotropic smoothing.")
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file();
}


void run () {

  Image::BufferPreload<float> src_data (argument[0]);
  Image::BufferPreload<float>::voxel_type src (src_data);

  std::vector<float> stdev (src_data.ndim(), 1);
  for (size_t dim = 3; dim < src_data.ndim(); dim++)
    stdev[dim] = 0;
  Options opt = get_options ("stdev");
  if (opt.size()) {
    stdev = parse_floats (opt[0][0]);
    for (size_t i = 0; i < stdev.size(); ++i)
      if (stdev[i] < 0.0)
        throw Exception ("the Gaussian stdev values cannot be negative");
    if (stdev.size() != 1 && stdev.size() != src_data.ndim())
      throw Exception ("the number of stdev elements does not correspond to the number of image dimensions");
  }

  std::vector<int> extent (1);
  extent[0] = 0;
  opt = get_options ("extent");
  if (opt.size()) {
    extent = parse_ints (opt[0][0]);
    if (extent.size() != 1 && extent.size() != src_data.ndim())
      throw Exception ("the number of extent elements does not correspond to the number of image dimensions");
    for (size_t i = 0; i < extent.size(); ++i) {
      if (!(extent[i] & int (1)))
        throw Exception ("expected odd number for extent");
      if (extent[i] < 0)
        throw Exception ("the kernel extent must be positive");
    }
  }

  Image::Filter::GaussianSmooth smooth_filter (src, extent, stdev);

  Image::Header header (src_data);
  header.info() = smooth_filter.info();
  header.datatype() = src_data.datatype();

  Image::Buffer<float> dest_data (argument[1], header);
  Image::Buffer<float>::voxel_type dest (dest_data);

  ProgressBar progress("smoothing image...");
  smooth_filter (src, dest);
}
