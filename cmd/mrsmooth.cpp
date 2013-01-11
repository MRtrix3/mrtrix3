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
            "The standard deviation is defined in mm (Default 1 voxel). "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the stdev for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the extent for each axis. "
            "The default extent is 2 * ceil(2.5 * stdev / voxel_size) - 1.")
  + Argument ("size").type_sequence_int()

  + Option ("anisotropic", "smooth each 3D volume of a 4D image using an anisotropic gaussian kernel "
                           "oriented along a corresponding direction. Note that when this option is used "
                           "the -stdev option takes two comma separated values, the first value defines the standard "
                           "deviation along the major eigenvector of the kernel (default: 1.27mm (FWHM = 3mm)) and "
                           "the second value defines the standard deviation along the other two eigenvectors "
                           "(default: 0.42mm (FWHM = 1mm)). This option is used to smooth AFD values sampled along a "
                           "number of directions. The directions can be specified within the header of the input image, "
                           "or by using the -directions option.")

  + Option ("directions", "the directions for anisotropic smoothing.")
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + Image::Stride::StrideOption;
}


void run () {

  Image::Header header (argument[0]);
  Image::BufferPreload<float> input_data (header);
  Image::BufferPreload<float>::voxel_type input_vox (input_data);

  bool do_anisotropic = false;
  Options opt = get_options ("anisotropic");
  if (opt.size())
    do_anisotropic = true;

  std::vector<float> stdev;
  if (do_anisotropic) {
    stdev.resize(2, 3.0 / 2.3548);
    stdev[1] = 1.0 / 2.3548;
  } else {
    stdev.resize(3);
    for (size_t dim = 0; dim < 3; dim++)
      stdev[dim] = input_data.vox (dim);
  }

  opt = get_options ("stdev");
  if (opt.size())
    stdev = parse_floats (opt[0][0]);

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
    Image::Filter::AnisotropicSmooth smooth_filter (input_vox, stdev, directions);
    header.info() = smooth_filter.info();
    if (strides.size()) {
      for (size_t n = 0; n < strides.size(); ++n)
        header.stride(n) = strides[n];
    }
    Image::Buffer<float> output_data (argument[1], header);
    Image::Buffer<float>::voxel_type output_vox (output_data);
    smooth_filter (input_vox, output_vox);

  } else {

    Image::Filter::GaussianSmooth<> smooth_filter (input_vox, stdev);
    opt = get_options ("extent");
    if (opt.size())
      smooth_filter.set_extent(parse_ints (opt[0][0]));

    header.info() = smooth_filter.info();
    if (strides.size()) {
      for (size_t n = 0; n < strides.size(); ++n)
        header.stride(n) = strides[n];
    }
    Image::Buffer<float> output_data (argument[1], header);
    Image::Buffer<float>::voxel_type output_vox (output_data);
    smooth_filter (input_vox, output_vox);
  }
}
