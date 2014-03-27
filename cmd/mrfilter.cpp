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
#include "image/filter/base.h"
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
// TODO Make sure the filters make use of the progress message member

// TODO Should all filters be templated?
// Alternatively, should the template on GaussianSmooth be removed?

// TODO Create maskfilter command, with erode / dilate / connected components / LCC / median
// Eventually this will form the basis for trying the fuzzy LCC algorithm
// Remember with fuzzy LCC command: wrap around image edges - that will hopefully make it
//   work with an inverted brain mask

// TODO Check erode and dilate filters: use bool class, use of RefPtr's

// TODO Check documentation of each filter; e.g. example use cases explicitly create
//   new header and manually set data type, this should be done within the filter constructor

// TODO Modify FFT to conform to code style, make available in mrfilter

// TODO Rename gaussian smooth to just 'smooth' (both file name and class)

// TODO Can LCC be removed completely? Just take the largest output of connected_components...

// TODO Rather than trying to provide a custom boolean implementation of the median() function,
//   just create a separate MedianBool image filter?

// TODO Rename Median3D to just Median? The namespace should differentiate it from the math function

// TODO Some filter headers have functions defined external to the filter class;
//   might be cleaner to have these within the relevant classes (unless they may have other applications)

// TODO The resize operator needs to be able to modify the underlying Info class
// For now, this remains unchanged i.e. doesn't derive from Filter::Base




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








void parse_gradient_filter_cmdline_options (Image::Filter::Gradient& filter)
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
    stdev.resize (filter.info().ndim(), 0.0);
    for (size_t dim = 0; dim != 3; ++dim)
      stdev[dim] = filter.info().vox (dim);
  }
  filter.set_stdev (stdev);

  opt = get_options ("scanner");
  filter.compute_wrt_scanner (opt.size());

}



void parse_median_filter_cmdline_options (Image::Filter::Median3D& filter)
{
  Options opt = get_options ("extent");
  if (opt.size())
    filter.set_extent (parse_ints (opt[0][0]));
}



void parse_smooth_filter_cmdline_options (Image::Filter::GaussianSmooth<>& filter)
{

  Options opt = get_options ("stdev");
  const bool stdev_supplied = opt.size();
  if (stdev_supplied)
    filter.set_stdev (parse_floats (opt[0][0]));

  opt = get_options ("fwhm");
  if (opt.size()) {
    if (stdev_supplied)
      throw Exception ("The stdev and FWHM options are mutually exclusive.");
    std::vector<float> stdevs = parse_floats (opt[0][0]);
    for (size_t d = 0; d < stdevs.size(); ++d)
      stdevs[d] = stdevs[d] / 2.3548;  //convert FWHM to stdev
    filter.set_stdev (stdevs);
  }

  opt = get_options ("extent");
  if (opt.size())
    filter.set_extent (parse_ints (opt[0][0]));

}





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

  const size_t filter_index = argument[1];

  Image::Filter::Base* filter = NULL;
  switch (filter_index) {
    case 0:
      filter = new Image::Filter::Gradient (input_voxel);
      parse_gradient_filter_cmdline_options (*(dynamic_cast<Image::Filter::Gradient*> (filter)));
      break;
    case 1:
      filter = new Image::Filter::Median3D (input_voxel);
      parse_median_filter_cmdline_options (*(dynamic_cast<Image::Filter::Median3D*> (filter)));
      break;
    case 2:
      filter = new Image::Filter::GaussianSmooth<> (input_voxel);
      parse_smooth_filter_cmdline_options (*(dynamic_cast<Image::Filter::GaussianSmooth<>* > (filter)));
      break;
    default:
      assert (0);
  }

  Image::Header header;
  header.info() = filter->info();

  Options opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > input_data.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n)
      header.stride(n) = strides[n];
  }

  Image::Buffer<float> output_data (argument[2], header);
  Image::Buffer<float>::voxel_type output_voxel (output_data);

  switch (filter_index) {
    case 0: (*dynamic_cast<Image::Filter::Gradient*>          (filter)) (input_voxel, output_voxel); break;
    case 1: (*dynamic_cast<Image::Filter::Median3D*>          (filter)) (input_voxel, output_voxel); break;
    case 2: (*dynamic_cast<Image::Filter::GaussianSmooth<>* > (filter)) (input_voxel, output_voxel); break;
  }

  delete filter;

}
