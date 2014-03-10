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

#include "command.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/gaussian_smooth.h"
#include "progressbar.h"



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
  + Argument ("mm").type_sequence_float()

  + Option ("fwhm", "apply Gaussian smoothing with the specified full-width half maximum. "
            "The FWHM is defined in mm (Default 1 voxel * 2.3548). "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the FWHM for each axis.")
  + Argument ("mm").type_sequence_float()

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the extent for each axis. "
            "The default extent is 2 * ceil(2.5 * stdev / voxel_size) - 1.")
  + Argument ("voxels").type_sequence_int()

  + Image::Stride::StrideOption;
}


void run () {

  Image::Header header (argument[0]);
  Image::BufferPreload<float> input_data (header);
  Image::BufferPreload<float>::voxel_type input_vox (input_data);

  std::vector<float> stdev;
  stdev.resize(3);
  for (size_t dim = 0; dim < 3; dim++)
    stdev[dim] = input_data.vox (dim);

  Options opt = get_options ("stdev");
  int stdev_supplied = opt.size();
  if (stdev_supplied) {
    stdev = parse_floats (opt[0][0]);
  }

  opt = get_options ("fwhm");
  if (opt.size()) {
    if (stdev_supplied)
      throw Exception ("The stdev and FWHM options are mutually exclusive.");
    stdev = parse_floats (opt[0][0]);
    for (size_t d = 0; d < stdev.size(); ++d)
      stdev[d] = stdev[d] / 2.3548;  //convert FWHM to stdev
  }

  opt = get_options ("stride");
  std::vector<int> strides;
  if (opt.size()) {
    strides = opt[0][0];
    if (strides.size() > header.ndim())
      throw Exception ("too many axes supplied to -stride option");
  }

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
