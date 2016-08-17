/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include <complex>

#include "command.h"
#include "image.h"
#include "filter/base.h"
#include "filter/fft.h"
#include "filter/gradient.h"
#include "filter/normalise.h"
#include "filter/median.h"
#include "filter/smooth.h"


using namespace MR;
using namespace App;


const char* filters[] = { "fft", "gradient", "median", "smooth", "normalise", NULL };


const OptionGroup FFTOption = OptionGroup ("Options for FFT filter")

  + Option ("axes", "the axes along which to apply the Fourier Transform. "
            "By default, the transform is applied along the three spatial axes. "
            "Provide as a comma-separate list of axis indices.")
    + Argument ("list").type_sequence_int()

  + Option ("inverse", "apply the inverse FFT")

  + Option ("magnitude", "output a magnitude image rather than a complex-valued image")

  + Option ("centre_zero", "re-arrange the FFT results so that the zero-frequency component "
            "appears in the centre of the image, rather than at the edges");



const OptionGroup GradientOption = OptionGroup ("Options for gradient filter")

  + Option ("stdev", "the standard deviation of the Gaussian kernel used to "
            "smooth the input image (in mm). The image is smoothed to reduced large "
            "spurious gradients caused by noise. Use this option to override "
            "the default stdev of 1 voxel. This can be specified either as a single "
            "value to be used for all 3 axes, or as a comma-separated list of "
            "3 values, one for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("magnitude", "output the gradient magnitude, rather "
            "than the default x,y,z components")

  + Option ("scanner", "define the gradient with respect to the scanner coordinate "
            "frame of reference.");


const OptionGroup MedianOption = OptionGroup ("Options for median filter")

  + Option ("extent", "specify extent of median filtering neighbourhood in voxels. "
        "This can be specified either as a single value to be used for all 3 axes, "
        "or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).")
    + Argument ("size").type_sequence_int();

const OptionGroup NormaliseOption = OptionGroup ("Options for normalisation filter")

  + Option ("extent", "specify extent of normalisation filtering neighbourhood in voxels. "
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



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "Perform filtering operations on 3D / 4D MR images. For 4D images, each 3D volume is processed independently."
  + "The available filters are: fft, gradient, median, smooth, normalise."
  + "Each filter has its own unique set of optional parameters.";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("filter", "the type of filter to be applied").type_choice (filters)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + FFTOption
  + GradientOption
  + MedianOption
  + NormaliseOption
  + SmoothOption
  + Stride::Options;
}


void run () {

  const size_t filter_index = argument[1];

  switch (filter_index) {

    // FFT
    case 0:
    {
      // FIXME Had to use cdouble throughout; seems to fail at compile time even trying to
      //   convert between cfloat and cdouble...
      auto input = Image<cdouble>::open (argument[0]).with_direct_io();
      Filter::FFT filter (input, get_options ("inverse").size());

      auto opt = get_options ("axes");
      if (opt.size())
        filter.set_axes (opt[0][0]);
      filter.set_centre_zero (get_options ("centre_zero").size());
      Stride::set_from_command_line (filter);
      filter.set_message (std::string("applying FFT filter to image " + std::string(argument[0])));

      if (get_options ("magnitude").size()) {
        auto temp = Image<cdouble>::scratch (filter, "complex FFT result");
        filter (input, temp);
        filter.datatype() = DataType::Float32;
        auto output = Image<float>::create (argument[2], filter);
        for (auto l = Loop (output) (temp, output); l; ++l)
          output.value() = std::abs (cdouble(temp.value()));
      } else {
        auto output = Image<cdouble>::create (argument[2], filter);
        filter (input, output);
      }
      break;
    }

    // Gradient
    case 1:
    {
      auto input = Image<float>::open (argument[0]);
      Filter::Gradient filter (input, get_options ("magnitude").size());

      std::vector<default_type> stdev;
      auto opt = get_options ("stdev");
      if (opt.size()) {
        stdev = parse_floats (opt[0][0]);
        for (size_t i = 0; i < stdev.size(); ++i)
          if (stdev[i] < 0.0)
            throw Exception ("the Gaussian stdev values cannot be negative");
        if (stdev.size() != 1 && stdev.size() != 3)
          throw Exception ("unexpected number of elements specified in Gaussian stdev");
      } else {
        stdev.resize (3, 0.0);
        for (size_t dim = 0; dim != 3; ++dim)
          stdev[dim] = filter.spacing (dim);
      }
      filter.compute_wrt_scanner (get_options ("scanner").size() ? true : false);
      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]));
      Stride::set_from_command_line (filter);
      filter.set_stdev (stdev);
      auto output = Image<float>::create (argument[2], filter);
      filter (input, output);
    break;
    }

    // Median
    case 2:
    {
      auto input = Image<float>::open (argument[0]);
      Filter::Median filter (input);

      auto opt = get_options ("extent");
      if (opt.size())
        filter.set_extent (parse_ints (opt[0][0]));
      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]));
      Stride::set_from_command_line (filter);

      auto output = Image<float>::create (argument[2], filter);
      filter (input, output);
      break;
     }

    // Smooth
    case 3:
    {
      auto input = Image<float>::open (argument[0]);
      Filter::Smooth filter (input);

      auto opt = get_options ("stdev");
      const bool stdev_supplied = opt.size();
      if (stdev_supplied)
        filter.set_stdev (parse_floats (opt[0][0]));
      opt = get_options ("fwhm");
      if (opt.size()) {
        if (stdev_supplied)
          throw Exception ("the stdev and FWHM options are mutually exclusive.");
        std::vector<default_type> stdevs = parse_floats((opt[0][0]));
        for (size_t d = 0; d < stdevs.size(); ++d)
          stdevs[d] = stdevs[d] / 2.3548;  //convert FWHM to stdev
        filter.set_stdev (stdevs);
      }
      opt = get_options ("extent");
      if (opt.size())
        filter.set_extent (parse_ints (opt[0][0]));
      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]));
      Stride::set_from_command_line (filter);

      auto output = Image<float>::create (argument[2], filter);
      filter (input, output);
      break;
    }

    // Normalisation
    case 4:
    {
      auto input = Image<float>::open (argument[0]);
      Filter::Normalise filter (input);

      auto opt = get_options ("extent");
      if (opt.size())
        filter.set_extent (parse_ints (opt[0][0]));
      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]) + "...");
      Stride::set_from_command_line (filter);

      auto output = Image<float>::create (argument[2], filter);
      filter (input, output);
      break;
     }

    default:
      assert (0);
      break;
  }
}
