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
#include "image/filter/anisotropic_smooth.h"
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
            "The default extent is ceil(2.5 * stdev_ / voxel_size standard deviations.")
  + Argument ("size").type_sequence_int()

  + Option ("anisotropic", "smooth each 3D volume of a 4D image using an anisotropic gaussian kernel "
                           "oriented along a corresponding direction. Note that when this option is used "
                           "the -stdev option defines the standard deviation along each eigenvector of the kernel."
                           "This can be used to smooth AFD values sampled along a number of directions. "
                           "The directions can be specified within the header of the input image, or by using the -directions option.")

  + Option ("directions", "the directions for anisotropic smoothing.")
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + Image::Stride::StrideOption;
}


void run () {

  Image::Header header (argument[0]);
  Image::BufferPreload<float> src_data (header);
  Image::BufferPreload<float>::voxel_type src (src_data);

  bool do_anisotropic = false;
  Options opt = get_options ("anisotropic");
  if (opt.size())
    do_anisotropic = true;

  std::vector<float> stdev;
  if (do_anisotropic) {
    stdev.resize(2, 3.0 / 2.3548);
    stdev[1] = 1.0 / 2.3548;
  } else {
    stdev.resize(1, 1);
  }

  opt = get_options ("stdev");
  if (opt.size()) {
    stdev = parse_floats (opt[0][0]);
    for (size_t i = 0; i < stdev.size(); ++i)
      if (stdev[i] < 0.0)
        throw Exception ("the Gaussian stdev values cannot be negative");
  }

  std::vector<int> extent (1,0);
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
  std::vector<int> radius = extent;
  for (size_t d = 0; d < radius.size(); ++d)
    radius[d] = (radius[d] - 1) / 2;

  opt = get_options ("stride");
  std::vector<int> strides;
  if (opt.size()) {
    strides = opt[0][0];
    if (strides.size() > header.ndim())
      throw Exception ("too many axes supplied to -stride option");
  }


  if (do_anisotropic) {
    Math::Matrix<float> directions;;
    opt = get_options ("directions");
    if (opt.size()) {
      directions.load(opt[0][0]);
    } else {
      if (!header["directions"].size())
        throw Exception ("no mask directions have been specified.");
      std::vector<float> dir_vector;
      std::vector<std::string > lines = split (header["directions"], "\n", true);
      for (size_t l = 0; l < lines.size(); l++) {
        std::vector<float> v (parse_floats(lines[l]));
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      directions.resize(dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        directions(i / 2, 0) = dir_vector[i];
        directions(i / 2, 1) = dir_vector[i + 1];
      }
    }
    Image::Filter::AnisotropicSmooth smooth_filter (src, stdev, directions);
    header.info() = smooth_filter.info();
    if (strides.size()) {
      for (size_t n = 0; n < strides.size(); ++n)
        header.stride(n) = strides[n];
    }
    Image::Buffer<float> dest_data (argument[1], header);
    Image::Buffer<float>::voxel_type dest (dest_data);
    smooth_filter (src, dest);
  } else {
    Image::Filter::GaussianSmooth smooth_filter (src, radius, stdev);
    header.info() = smooth_filter.info();
    if (strides.size()) {
      for (size_t n = 0; n < strides.size(); ++n)
        header.stride(n) = strides[n];
    }
    Image::Buffer<float> dest_data (argument[1], header);
    Image::Buffer<float>::voxel_type dest (dest_data);
    smooth_filter (src, dest);
  }
}
