/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <complex>
#include <set>

#include "command.h"
#include "filter/base.h"
#include "filter/demodulate.h"
#include "filter/gradient.h"
#include "filter/kspace.h"
#include "filter/median.h"
#include "filter/normalise.h"
#include "filter/smooth.h"
#include "filter/zclean.h"
#include "image.h"
#include "interp/cubic.h"
#include "math/fft.h"

using namespace MR;
using namespace App;

const std::vector<std::string> filters = {
    "demodulate", "fft", "gradient", "kspace", "median", "smooth", "normalise", "zclean"};

// clang-format off
const OptionGroup FFTAxesOption = OptionGroup ("Options applicable to demodulate / FFT / k-space filters")
  + Option ("axes", "the axes along which to apply the Fourier Transform."
                    " By default, the transform is applied along the three spatial axes."
                    " Provide as a comma-separate list of axis indices.")
    + Argument ("list").type_sequence_int();

const OptionGroup DemodulateOption = OptionGroup ("Options applicable to demodulate filter")
  + Option ("linear", "only demodulate based on a linear phase ramp, "
                      "rather than a filtered k-space");

const OptionGroup FFTOption = OptionGroup ("Options for FFT filter")
  + Option ("inverse", "apply the inverse FFT")
  + Option ("magnitude", "output a magnitude image rather than a complex-valued image")
  + Option ("rescale", "rescale values so that inverse FFT recovers original values")
  + Option ("centre_zero", "re-arrange the FFT results so that"
                           " the zero-frequency component appears in the centre of the image,"
                           " rather than at the edges");

const OptionGroup GradientOption = OptionGroup ("Options for gradient filter")
  + Option ("stdev", "the standard deviation of the Gaussian kernel used to "
                     " smooth the input image (in mm)."
                     " The image is smoothed to reduced large spurious gradients caused by noise."
                     " Use this option to override the default stdev of 1 voxel."
                     " This can be specified either as a single value to be used for all 3 axes,"
                     " or as a comma-separated list of 3 values (one for each axis).")
  + Argument ("sigma").type_sequence_float()
  + Option ("magnitude", "output the gradient magnitude,"
                         " rather than the default x,y,z components")
  + Option ("scanner", "define the gradient with respect to"
                       " the scanner coordinate frame of reference.");

const OptionGroup KSpaceOption = OptionGroup ("Options for k-space filtering")
  + Option ("window", "specify the shape of the k-space window filter; "
                      "options are: " + join(Filter::kspace_window_choices, ",") + " "
                      "(no default; must be specified for \"kspace\" operation)")
    + Argument("name").type_choice(Filter::kspace_window_choices)
  + Option ("strength", "modulate the strength of the chosen filter "
                        "(exact interpretation & defaultmay depend on the exact filter chosen)")
    + Argument("value").type_float(0.0, 1.0);

const OptionGroup MedianOption = OptionGroup ("Options for median filter")
  + Option ("extent", "specify extent of median filtering neighbourhood in voxels."
                      " This can be specified either as a single value to be used for all 3 axes,"
                      " or as a comma-separated list of 3 values (one for each axis)"
                      " (default: 3x3x3).")
    + Argument ("size").type_sequence_int();

const OptionGroup NormaliseOption = OptionGroup ("Options for normalisation filter")
  + Option ("extent", "specify extent of normalisation filtering neighbourhood in voxels."
                      "This can be specified either as a single value to be used for all 3 axes,"
                      "or as a comma-separated list of 3 values (one for each axis)"
                      " (default: 3x3x3).")
    + Argument ("size").type_sequence_int();

const OptionGroup SmoothOption = OptionGroup ("Options for smooth filter")
  + Option ("stdev", "apply Gaussian smoothing with the specified standard deviation."
                     " The standard deviation is defined in mm (Default 1 voxel)."
                     " This can be specified either as a single value to be used for all axes,"
                     " or as a comma-separated list of the stdev for each axis.")
    + Argument ("mm").type_sequence_float()
  + Option ("fwhm", "apply Gaussian smoothing with the specified full-width half maximum."
                    " The FWHM is defined in mm (Default 1 voxel * 2.3548)."
                    " This can be specified either as a single value to be used for all axes,"
                    " or as a comma-separated list of the FWHM for each axis.")
  + Argument ("mm").type_sequence_float()
  + Option ("extent", "specify the extent (width) of kernel size in voxels."
                      " This can be specified either as a single value to be used for all axes,"
                      " or as a comma-separated list of the extent for each axis."
                      " The default extent is 2 * ceil(2.5 * stdev / voxel_size) - 1.")
  + Argument ("voxels").type_sequence_int();

const OptionGroup ZcleanOption = OptionGroup ("Options for zclean filter")
  + Option ("zupper", "define high intensity outliers;"
                      " default: 2.5")
    + Argument ("num").type_float(0.1)
  + Option ("zlower", "define low intensity outliers;"
                      " default: 2.5")
    + Argument ("num").type_float(0.1)
  + Option ("bridge", "number of voxels to gap to fill holes in mask;"
                      " default: 4")
    + Argument ("num").type_integer(0)
  + Option ("maskin", "initial mask that defines the maximum spatial extent"
                      " and the region from which to smaple the intensity range.")
    + Argument ("image").type_image_in()
  + Option ("maskout", "Output a refined mask based on a spatially coherent region"
                       " with normal intensity range.")
    + Argument ("image").type_image_out();


void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)"
           " and David Raffelt (david.raffelt@florey.edu.au)"
           " and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Perform filtering operations on 3D / 4D MR images";

  DESCRIPTION
  + "The available filters are:"
    " demodulate, fft, gradient, median, smooth, normalise, zclean."
  + "Each filter has its own unique set of optional parameters."
  + "For 4D images, each 3D volume is processed independently.";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("filter", "the type of filter to be applied").type_choice (filters)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + FFTAxesOption
  + DemodulateOption
  + FFTOption
  + GradientOption
  + KSpaceOption
  + MedianOption
  + NormaliseOption
  + SmoothOption
  + ZcleanOption
  + Stride::Options;

}
// clang-format on

// TODO Use presence of SliceEncodingDirection to select defaults
std::vector<size_t> get_axes(const Header &H, const std::vector<size_t> &default_axes) {
  auto opt = get_options("axes");
  std::vector<size_t> axes = default_axes;
  if (!opt.empty()) {
    axes = parse_ints<size_t>(opt[0][0]);
    for (const auto axis : axes)
      if (axis >= H.ndim())
        throw Exception("axis provided with -axes option is out of range");
    if (std::set<size_t>(axes.begin(), axes.end()).size() != axes.size())
      throw Exception("axis indices must not contain duplicates");
  }
  return axes;
}

void run() {

  const size_t filter_index = argument[1];

  switch (filter_index) {

  // Phase demodulation
  case 0: {
    Header H_in = Header::open(argument[0]);
    if (!H_in.datatype().is_complex())
      throw Exception("demodulation filter only applicable for complex image data");
    auto input = H_in.get_image<cdouble>();

    const std::vector<size_t> inner_axes = get_axes(H_in, {0, 1});

    Filter::Demodulate filter(input, inner_axes, !get_options("linear").empty());

    Header H_out(H_in);
    Stride::set_from_command_line(H_out);
    auto output = Image<cdouble>::create(argument[2], H_out);

    filter(input, output);
  } break;

  // FFT
  case 1: {
    Header H_in = Header::open(argument[0]);
    std::vector<size_t> axes = get_axes(H_in, {0, 1, 2});
    const int direction = get_options("inverse").empty() ? FFTW_FORWARD : FFTW_BACKWARD;
    const bool centre_FFT = !get_options("centre_zero").empty();
    const bool magnitude = !get_options("magnitude").empty();

    Header H_out(H_in);
    Stride::set_from_command_line(H_out);
    H_out.datatype() = magnitude ? DataType::Float32 : DataType::CFloat64;
    auto output = Image<cdouble>::create(argument[2], H_out);
    double scale = 1.0;

    // FIXME Had to use cdouble throughout; seems to fail at compile time even trying to
    //   convert between cfloat and cdouble...
    auto input = H_in.get_image<cdouble>();

    Image<cdouble> in(input), out;
    for (size_t n = 0; n < axes.size(); ++n) {
      scale *= in.size(axes[n]);
      if (n == (axes.size() - 1) && !magnitude) {
        out = output;
      } else if (!out.valid()) {
        out = Image<cdouble>::scratch(H_in);
      }

      Math::FFT(in, out, axes[n], direction, centre_FFT);

      in = out;
    }

    if (magnitude) {
      ThreadedLoop(out).run(
          [](decltype(out) &a, decltype(output) &b) { a.value() = abs(cdouble(b.value())); }, output, out);
    }
    if (!get_options("rescale").empty()) {
      scale = 1.0 / std::sqrt(scale);
      ThreadedLoop(out).run([&scale](decltype(out) &a) { a.value() *= scale; }, output);
    }

    break;
  }

  // Gradient
  case 2: {
    auto input = Image<float>::open(argument[0]);
    Filter::Gradient filter(input, !get_options("magnitude").empty());

    std::vector<default_type> stdev;
    auto opt = get_options("stdev");
    if (!opt.empty()) {
      stdev = parse_floats(opt[0][0]);
      for (size_t i = 0; i < stdev.size(); ++i)
        if (stdev[i] < 0.0)
          throw Exception("the Gaussian stdev values cannot be negative");
      if (stdev.size() != 1 && stdev.size() != 3)
        throw Exception("unexpected number of elements specified in Gaussian stdev");
    } else {
      stdev.resize(3, 0.0);
      for (size_t dim = 0; dim != 3; ++dim)
        stdev[dim] = filter.spacing(dim);
    }
    filter.compute_wrt_scanner(!get_options("scanner").empty());
    filter.set_message(std::string("applying ") + std::string(argument[1]) + " filter" + //
                       " to image " + std::string(argument[0]));
    Stride::set_from_command_line(filter);
    filter.set_stdev(stdev);
    auto output = Image<float>::create(argument[2], filter);
    filter(input, output);
    break;
  }

  // k-space filtering
  case 3: {
    auto opt_window = get_options("window");
    if (opt_window.empty())
      throw Exception("-window option is compulsory for k-space filtering");

    Header H_in = Header::open(argument[0]);
    const std::vector<size_t> axes = get_axes(H_in, {0, 1, 2});
    const bool is_complex = H_in.datatype().is_complex();
    auto input = H_in.get_image<cdouble>();

    Image<double> window;
    switch (Filter::kspace_windowfn_t(int(opt_window[0][0]))) {
    case Filter::kspace_windowfn_t::TUKEY:
      window = Filter::KSpace::window_tukey(H_in, axes, get_option_value("strength", Filter::default_tukey_width));
      break;
    default:
      assert(false);
    }
    Filter::KSpace filter(H_in, window);
    Header H_out(H_in);

    if (is_complex) {
      auto output = Image<cdouble>::create(argument[2], H_out);
      filter(input, output);
    } else {
      H_out.datatype() = DataType::Float32;
      H_out.datatype().set_byte_order_native();
      auto output = Image<float>::create(argument[2], H_out);
      filter(input, output);
    }
    break;
  }

  // Median
  case 4: {
    auto input = Image<float>::open(argument[0]);
    Filter::Median filter(input);

    auto opt = get_options("extent");
    if (!opt.empty())
      filter.set_extent(parse_ints<uint32_t>(opt[0][0]));
    filter.set_message(std::string("applying ") + std::string(argument[1]) + " filter" + //
                       " to image " + std::string(argument[0]));
    Stride::set_from_command_line(filter);

    auto output = Image<float>::create(argument[2], filter);
    filter(input, output);
    break;
  }

  // Smooth
  case 5: {
    auto input = Image<float>::open(argument[0]);
    Filter::Smooth filter(input);

    auto opt = get_options("stdev");
    const bool stdev_supplied = !opt.empty();
    if (stdev_supplied)
      filter.set_stdev(parse_floats(opt[0][0]));
    opt = get_options("fwhm");
    if (!opt.empty()) {
      if (stdev_supplied)
        throw Exception("the stdev and FWHM options are mutually exclusive.");
      std::vector<default_type> stdevs = parse_floats((opt[0][0]));
      for (size_t d = 0; d < stdevs.size(); ++d)
        stdevs[d] = stdevs[d] / 2.3548; // convert FWHM to stdev
      filter.set_stdev(stdevs);
    }
    opt = get_options("extent");
    if (!opt.empty())
      filter.set_extent(parse_ints<uint32_t>(opt[0][0]));
    filter.set_message(std::string("applying ") + std::string(argument[1]) + " filter" + //
                       " to image " + std::string(argument[0]));
    Stride::set_from_command_line(filter);

    auto output = Image<float>::create(argument[2], filter);
    threaded_copy(input, output);
    filter(output);
    break;
  }

  // Normalisation
  case 6: {
    auto input = Image<float>::open(argument[0]);
    Filter::Normalise filter(input);

    auto opt = get_options("extent");
    if (!opt.empty())
      filter.set_extent(parse_ints<uint32_t>(opt[0][0]));
    filter.set_message(std::string("applying ") + std::string(argument[1]) + " filter" + //
                       " to image " + std::string(argument[0]));
    Stride::set_from_command_line(filter);

    auto output = Image<float>::create(argument[2], filter);
    filter(input, output);
    break;
  }

  // Zclean
  case 7: {
    auto input = Image<float>::open(argument[0]);
    Filter::ZClean filter(input);

    auto opt = get_options("maskin");
    if (opt.empty())
      throw Exception(std::string(argument[1]) + " filter requires initial mask");
    Image<float> maskin = Image<float>::open(opt[0][0]);
    check_dimensions(maskin, input, 0, 3);

    filter.set_message(std::string("applying ") + std::string(argument[1]) + " filter" + //
                       " to image " + std::string(argument[0]));
    Stride::set_from_command_line(filter);

    filter.set_voxels_to_bridge(get_option_value("bridge", 4));
    float zlower = get_option_value("zlower", 2.5);
    float zupper = get_option_value("zupper", 2.5);
    filter.set_zlim(zlower, zupper);

    auto output = Image<float>::create(argument[2], filter);
    filter(input, maskin, output);

    opt = get_options("maskout");
    if (!opt.empty()) {
      auto maskout = Image<bool>::create(opt[0][0], filter.mask);
      threaded_copy(filter.mask, maskout);
    }
    break;
  }

  default:
    assert(0);
    break;
  }
}
