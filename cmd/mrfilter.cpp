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
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/filter/gaussian_smooth.h"
#include "image/filter/gradient.h"
#include "image/filter/median3D.h"


using namespace MR;
using namespace App;



// TODO Create an Image::Filter::Base class, so the filter can be constructed based on
//   a base pointer, set up in a code branch, and then a single execution line is used.
// Can also set up progress message as a member variable before functor is used; will
//   behave differently depending on whether input is 3D or 4D; in fact, could possibly
//   make this a base function
// Extent should also possibly be a member of the base class; should be a common
//   feature of image 'filters'

// TODO For gradient filter, move smoothing filter operation to inside gradient filter

// TODO Should all filters be templated?




const char* filters[] = { "gradient", "median", "smooth", NULL };




const OptionGroup GradientOption = OptionGroup ("Options for gradient filter")

  + Option ("stdev", "the standard deviation of the Gaussian kernel used to "
            "smooth the input image (in mm). The image is smoothed to reduced large "
            "spurious gradients caused by noise. Use this option to override "
            "the default stdev of 1 voxel. This can be specified either as a single "
            "value to be used for all 3 axes, or as a comma-separated list of "
            "3 values, one for each axis.")
  + Argument ("sigma").type_sequence_float()

  // TODO Implement
  + Option ("greyscale", "output the image gradient as a greyscale image, rather "
            "than the default x,y,z components")

  + Option ("scanner", "compute the gradient with respect to the scanner coordinate "
            "frame of reference.");




const OptionGroup MedianOption = OptionGroup ("Options for median filter")

  + Option ("extent", "specify extent of median filtering neighbourhood in voxels. "
        "This can be specified either as a single value to be used for all 3 axes, "
        "or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).")
    + Argument ("size").type_sequence_int();





const OptionGroup SmoothOption = OptionGroup ("Options for smooth filter")

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
  + Argument ("voxels").type_sequence_int();








void parse_gradient_filter_cmdline_options (Image::Filter::GaussianSmooth<>& smooth_filter, Image::Filter::Gradient& gradient_filter)
{

  std::vector<float> stdev;
  Options opt = get_options ("stdev");
  if (opt.size()) {
    stdev = parse_floats (opt[0][0]);
    for (size_t i = 0; i < stdev.size(); ++i)
      if (stdev[i] < 0.0)
        throw Exception ("the Gaussian stdev values cannot be negative");
    if (stdev.size() != 1 && stdev.size() != 3)
      throw Exception ("unexpected number of elements specified in Gaussian stdev");
  } else {
    stdev.resize (smooth_filter.info().ndim(), 0.0);
    for (size_t dim = 0; dim != 3; ++dim)
      stdev[dim] = smooth_filter.info().vox (dim);
  }
  smooth_filter.set_stdev (stdev);

  opt = get_options ("scanner");
  gradient_filter.compute_wrt_scanner (opt.size());

}



// TODO Functions for importing command-line options for the other filters
// Keep the option groups and these functions in the cmd/ file; only really relevant for this command, other
//   uses of the classes are likely to be deeper in a processing chain






void usage ()
{
  AUTHOR = "Robert E. Smith (r.smith@brain.org.au), David Raffelt (d.raffelt@brain.org.au) and J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "Perform filtering operations on 3D / 4D MR images. For 4D images, each 3D volume is processed independently."

  + "The available filters are: gradient, median, smooth."

  + "Each filter has its own unique set of optional parameters.";


  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("filter", "the type of filter to be applied").type_choice (filters)
  + Argument ("output", "the output image.").type_image_out ();


  OPTIONS
  + GradientOption
  + MedianOption
  + SmoothOption

  + Image::Stride::StrideOption;

}


void run () {

  Image::BufferPreload<float> input_data (argument[0]);
  Image::BufferPreload<float>::voxel_type input_voxel (input_data);

  if (int(argument[1]) == 0) { // Gradient filter

    Image::Filter::GaussianSmooth<> smooth_filter (input_voxel);
    Image::Filter::Gradient gradient_filter (input_voxel);

    Image::Header smooth_header (input_data);
    smooth_header.info() = smooth_filter.info();

    Image::BufferScratch<float> smoothed_data (smooth_header);
    Image::BufferScratch<float>::voxel_type smoothed_voxel (smoothed_data);

    Image::Header gradient_header (input_data);
    gradient_header.info() = gradient_filter.info();

    Image::Buffer<float> gradient_data (argument[2], gradient_header);
    Image::Buffer<float>::voxel_type gradient_voxel (gradient_data);

    ProgressBar progress ("computing image gradient...");
    smooth_filter (input_voxel, smoothed_voxel);
    gradient_filter (smoothed_voxel, gradient_voxel);

  } else if (int(argument[1]) == 1) { // Median filter

    std::vector<int> extent (1);
    extent[0] = 3;

    Options opt = get_options ("extent");

    if (opt.size())
      extent = parse_ints (opt[0][0]);

    Image::BufferPreload<float> src_array (argument[0]);
    Image::BufferPreload<float>::voxel_type src (src_array);

    Image::Filter::Median3D median_filter (src, extent);

    Image::Header header (src_array);
    header.info() = median_filter.info();
    header.datatype() = src_array.datatype();

    Image::Buffer<float> dest_array (argument[2], header);
    Image::Buffer<float>::voxel_type dest (dest_array);

    median_filter (src, dest);

  } else if (int(argument[1]) == 2) { // Smooth filter

    std::vector<float> stdev;
    stdev.resize(3);
    for (size_t dim = 0; dim < 3; dim++)
      stdev[dim] = input_data.vox (dim);

    Options opt = get_options ("stdev");
    const bool stdev_supplied = opt.size();
    if (stdev_supplied)
      stdev = parse_floats (opt[0][0]);

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
      if (strides.size() > input_data.ndim())
        throw Exception ("too many axes supplied to -stride option");
    }

    Image::Filter::GaussianSmooth<> smooth_filter (input_voxel, stdev);
    opt = get_options ("extent");
    if (opt.size())
      smooth_filter.set_extent (parse_ints (opt[0][0]));

    // TODO Allow setting of strides for all filters
    Image::Header header;
    header.info() = smooth_filter.info();
    if (strides.size()) {
      for (size_t n = 0; n < strides.size(); ++n)
        header.stride(n) = strides[n];
    }
    Image::Buffer<float> output_data (argument[1], header);
    Image::Buffer<float>::voxel_type output_voxel (output_data);
    smooth_filter (input_voxel, output_voxel);

  } else {
    assert (0);
  }


}
