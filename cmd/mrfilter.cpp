/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include <array>
#include <complex>

#include "command.h"
#include "image.h"
#include "adapter/base.h"
#include "adapter/edge_handlers.h"
#include "adapter/kernel.h"
#include "filter/base.h"
#include "filter/kernels.h"
#include "filter/fft.h"
#include "filter/gradient.h"
#include "filter/laplacian.h"
#include "filter/normalise.h"
#include "filter/median.h"
#include "filter/smooth.h"
#include "filter/zclean.h"


using namespace MR;
using namespace App;

enum filter_t {            BOXBLUR,   FARID,   FFT,   GAUSSIAN,   LAPLACIAN3D,   MEDIAN,   RADIALBLUR,   SOBEL,   SOBELFELDMAN,   SHARPEN,   UNSHARPMASK,   ZCLEAN };
const char* filters[] = { "boxblur", "farid", "fft", "gaussian", "laplacian3d", "median", "radialblur", "sobel", "sobelfeldman", "sharpen", "unsharpmask", "zclean", nullptr };
// TODO Remaining: gradient, laplacian1d, smooth

const std::string filter_list_description = "The available filters are: " + join (filters, ", ") + ".";


// TODO Should smoothing prior to filter operation be possible for filters other than gradient?
// TODO Option to rotate sobel / sobel-feldman result to scanner space?
//   What about more generally scaling kernels based on anisotropic voxel sizes?


const OptionGroup BoxblurOption = OptionGroup ("Options for boxblur filter")

  + Option ("extent", "specify extent of boxblur filtering kernel in voxels. "
        "This can be specified either as a single value to be used for all 3 axes, "
        "or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).")
    + Argument ("size").type_sequence_int();


const OptionGroup FaridOption = OptionGroup ("Options for farid filter")

  + Option ("order", "specify order of derivative (default: 1st order i.e. gradient)")
    + Argument ("value").type_integer (1, 3)
  + Option ("extent", "specify width of filter kernel (default: 1 + (2 x order)) (must be odd)")
    + Argument ("value").type_sequence_int()
  + Option ("magnitude", "output norm magnitude of filter result rather than 3 spatial components");



const OptionGroup FFTOption = OptionGroup ("Options for FFT filter")

  + Option ("axes", "the axes along which to apply the Fourier Transform. "
            "By default, the transform is applied along the three spatial axes. "
            "Provide as a comma-separate list of axis indices.")
    + Argument ("list").type_sequence_int()

  + Option ("inverse", "apply the inverse FFT")

  + Option ("magnitude", "output a magnitude image rather than a complex-valued image")

  + Option ("centre_zero", "re-arrange the FFT results so that the zero-frequency component "
            "appears in the centre of the image, rather than at the edges");




const OptionGroup GaussianOption = OptionGroup ("Options for Gaussian filter")

  + Option ("fwhm", "set the Full-Width at Half-Maximum (FWHM) of the kernel in mm "
                    "(default: 1 voxel)")
    + Argument ("value").type_float (0.0)

  + Option ("radius", "set the radius in mm beyond which the kernel will be truncated "
                      "(default: 3 x FWHM)")
    + Argument ("value").type_float (0.0);




/*
const OptionGroup GradientLaplaceOption = OptionGroup ("Options for gradient & laplacian1d filters")

  + Option ("stdev", "the standard deviation of the Gaussian kernel used to "
            "smooth the input image (in mm). The image is smoothed to reduced large "
            "spurious gradients caused by noise. Use this option to override "
            "the default stdev of 1 voxel. This can be specified either as a single "
            "value to be used for all 3 axes, or as a comma-separated list of "
            "3 values, one for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("magnitude", "output a magnitude image, rather "
            "than the default x,y,z components")

  + Option ("scanner", "define the relevant derivative with respect to the scanner coordinate "
            "frame of reference.");
*/




const OptionGroup MedianOption = OptionGroup ("Options for median filter")

  + Option ("extent", "specify extent of median filtering neighbourhood in voxels. "
        "This can be specified either as a single value to be used for all 3 axes, "
        "or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).")
    + Argument ("size").type_sequence_int();



const OptionGroup RadialblurOption = OptionGroup ("Options for radialblur filter")

  + Option ("radius", "specify radius of blurring kernel in mm. "
                      "(Default: 2 voxels along axis with largest voxel spacing)")
    + Argument ("value").type_float(0.0);



const OptionGroup SobelOption = OptionGroup ("Options for sobel and sobelfeldman filters")
  + Option ("magnitude", "output norm magnitude of filter result rather than 3 spatial components");


/*
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
*/



const OptionGroup SharpenOption = OptionGroup ("Options for sharpen filter")

  + Option ("strength", "specify the strength of the sharpening kernel. "
                        "(default = 1.0)")
    + Argument ("value").type_float(0.0);



const OptionGroup UnsharpMaskOption = OptionGroup ("Options for UnsharpMask filter")

  + Option ("fwhm", "set the Full-Width at Half-Maximum (FWHM) of the smoothing kernel in mm "
                    "(default: 2 voxels)")
    + Argument ("value").type_float (0.0)

  + Option ("strength", "set the strength of the edge enhancement "
                      "(default: 1.0)")
    + Argument ("value").type_float (0.0);



const OptionGroup ZcleanOption = OptionGroup ("Options for zclean filter")
+ Option ("zupper", "define high intensity outliers: default: 2.5")
  + Argument ("num").type_float(0.1, std::numeric_limits<float>::infinity())
+ Option ("zlower", "define low intensity outliers: default: 2.5")
  + Argument ("num").type_float(0.1, std::numeric_limits<float>::infinity())
+ Option ("bridge", "number of voxels to gap to fill holes in mask: default: 4")
  + Argument ("num").type_integer(0)
+ Option ("maskin", "initial mask that defines the maximum spatial extent and the region from "
          "which to sample the intensity range.")
  + Argument ("image").type_image_in()
+ Option ("maskout", "Output a refined mask based on a spatially coherent region with normal intensity range.")
  + Argument ("image").type_image_out();






void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Perform filtering operations on 3D / 4D MR images";

  DESCRIPTION
  + filter_list_description.c_str()
  + "Each filter has its own unique set of optional parameters."
  + "For 4D images, each 3D volume is processed independently.";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("filter", "the type of filter to be applied").type_choice (filters)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + BoxblurOption
  + FaridOption
  + FFTOption
  + GaussianOption
  //+ GradientLaplaceOption
  + MedianOption
  + RadialblurOption
  //+ SmoothOption
  + SobelOption
  + SharpenOption
  + UnsharpMaskOption
  + ZcleanOption
  + Stride::Options;
}


template <class HeaderType>
default_type voxel_size (const HeaderType& header)
{
  return std::cbrt (header.spacing (0) * header.spacing (1) * header.spacing (2));
}






vector<int> parse_extent()
{
  auto opt = get_options ("extent");
  if (!opt.size())
    return vector<int>();
  auto extent = parse_ints (opt[0][0]);
  switch (extent.size()) {
    case 1:
      if (!(extent[0] & int(1)))
        throw Exception ("Kernel extent must be an odd number");
      return { extent[0], extent[0], extent[0] };
    case 3:
      for (auto e : extent) {
        if (!(e & int(1)))
          throw Exception ("All kernel extents must be odd numbers");
      }
      return extent;
    default:
      throw Exception ("Kernel extent must be either a single scalar number, or three comma-separated values");
  }
}





// TODO Completely re-write run() from scratch
// TODO Some are not multi-threaded
void run ()
{
  Header input_header (Header::open (argument[0]));
  const size_t filter_index = argument[1];

  switch (filter_index) {

    case FFT:
    {
      // FIXME Had to use cdouble throughout; seems to fail at compile time even trying to
      //   convert between cfloat and cdouble...
      auto input = input_header.get_image<cdouble>().with_direct_io();
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
          output.value() = abs (cdouble(temp.value()));
      } else {
        auto output = Image<cdouble>::create (argument[2], filter);
        filter (input, output);
      }
      break;
    }

    case BOXBLUR:
    case LAPLACIAN3D:
    case RADIALBLUR:
    case SHARPEN:
    case UNSHARPMASK:
    {
      Image<float> input = input_header.get_image<float>().with_direct_io();
      Adapter::EdgeExtend<Image<float>> edge_adapter (input);
      Filter::Kernels::kernel_type kernel;
      switch (filter_index) {
        case BOXBLUR:
          {
            auto extent = parse_extent();
            if (extent.size() == 1) {
              kernel = Filter::Kernels::boxblur (extent[0]);
            } else if (extent.size() == 3) {
              kernel = Filter::Kernels::boxblur (extent);
            } else {
              kernel = Filter::Kernels::boxblur (3);
            }
          }
          break;
        case LAPLACIAN3D:
          kernel = Filter::Kernels::laplacian3d();
          break;
        case RADIALBLUR:
          kernel = Filter::Kernels::radialblur (input_header, get_option_value ("radius", 2.0 * std::max({input_header.spacing(0), input_header.spacing(1), input_header.spacing(2)})));
          break;
        case SHARPEN:
          kernel = Filter::Kernels::sharpen (get_option_value ("strength", 1.0));
          break;
        case UNSHARPMASK:
          kernel = Filter::Kernels::unsharp_mask (input_header,
                                                  get_option_value ("fwhm", 2.0 * voxel_size (input_header)),
                                                  get_option_value ("strength", 1.0));
          break;
        default:
          assert (0);
      }
      Adapter::Kernel::Single<decltype(edge_adapter)> kernel_adapter (edge_adapter, kernel);
      Image<float> output (Image<float>::create (argument[2], kernel_adapter));
      copy_with_progress_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]),
                                  kernel_adapter, output);
    }
    break;

    case GAUSSIAN:
    {
      Image<float> input = input_header.get_image<float>().with_direct_io();
      // Different edge handler to other filters
      Adapter::EdgeCrop<Image<float>> edge_adapter (input, 0.0f);
      const float fwhm = get_option_value ("fwhm", float (std::cbrt (input.spacing (0) * input.spacing(1) * input.spacing(2))));
      const float radius = get_option_value ("radius", 3.0f * fwhm);
      auto kernel = Filter::Kernels::gaussian (input, fwhm, radius);
      Adapter::Kernel::Single<decltype(edge_adapter)> kernel_adapter (edge_adapter, kernel);
      Image<float> output (Image<float>::create (argument[2], kernel_adapter));
      copy_with_progress_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]),
                                  kernel_adapter, output);
    }
    break;

    case MEDIAN:
    {
      auto input = input_header.get_image<float>().with_direct_io();
      Filter::Median filter (input);

      auto extent = parse_extent();
      if (extent.size())
        filter.set_extent (extent);
      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]));
      Stride::set_from_command_line (filter);

      auto output = Image<float>::create (argument[2], filter);
      filter (input, output);
    }
    break;

    case FARID:
    case SOBEL:
    case SOBELFELDMAN:
    {
      Image<float> input = input_header.get_image<float>().with_direct_io();
      Adapter::EdgeExtend<Image<float>> edge_adapter (input);
      std::array<Filter::Kernels::kernel_type, 3> kernels;
      switch (filter_index) {
        case FARID:
        {
          const size_t order = get_option_value ("order", 1);
          auto opt = get_options ("extent");
          size_t extent;
          if (opt.size()) {
            auto extents = parse_ints (opt[0][0]);
            if (extents.size() != 1)
              throw Exception ("Farid filter requires a single extent value for all axes");
            extent = extents[0];
          } else {
            extent = 1 + (2 * order);
          }
          kernels = Filter::Kernels::farid (order, extent); break;
        }
        case SOBEL:        kernels = Filter::Kernels::sobel();         break;
        case SOBELFELDMAN: kernels = Filter::Kernels::sobel_feldman(); break;
        default: assert (0);
      }
      if (get_options ("magnitude").size()) {
        Adapter::Kernel::TripletNorm<decltype(edge_adapter)> kernel_adapter (edge_adapter, kernels);
        Image<float> output (Image<float>::create (argument[2], kernel_adapter));
        copy_with_progress_message (std::string("applying norm of ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]),
                                    kernel_adapter, output);
      } else {
        Adapter::Kernel::Triplet<decltype(edge_adapter)> kernel_adapter (edge_adapter, kernels);
        Image<float> output (Image<float>::create (argument[2], kernel_adapter));
        copy_with_progress_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]),
                                    kernel_adapter, output);
      }
    }
    break;

    case ZCLEAN:
    {
      auto input = input_header.get_image<float>().with_direct_io();
      Filter::ZClean filter (input);

      auto opt = get_options ("maskin");
      if (!opt.size())
        throw Exception (std::string(argument[1]) + " filter requires initial mask");
      Image<float> maskin = Image<float>::open (opt[0][0]);
      check_dimensions (maskin, input, 0, 3);

      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]) + "...");
      Stride::set_from_command_line (filter);

      filter.set_voxels_to_bridge (get_option_value ("bridge", 4));
      filter.set_zlim (get_option_value ("zlower", 2.5), get_option_value ("zupper", 2.5));

      auto output = Image<float>::create (argument[2], filter);
      filter (input, maskin, output);

      opt = get_options ("maskout");
      if (opt.size()) {
        auto maskout = Image<bool>::create (opt[0][0], filter.mask);
        threaded_copy (filter.mask, maskout);
      }
    }
    break;


/*
    // Gradient
    case 1:
    {
      // TODO This is a 1D gradient being applied in 3 axes, with pre-smoothing

      const bool wrt_scanner = get_options ("scanner").size();
      const bool magnitude = get_options ("magnitude").size();

      Adapter::EdgeExtend<Image<float>> edge_adapter (Image<float>::open (argument[0]).with_direct_io());
      // TODO Need explicit pre-smoothing
      Adapter::Gradient1D<decltype(edge_adapter)> gradient_1D (edge_adapter, 0, false);
      Adapter::BaseFiniteDiff3D<decltype(gradient_1D)> gradient_3D (gradient_1D, wrt_scanner);
      // TODO Major branch based on whether the magnitude is to be taken
      if (magnitude) {
        Filter::NormWrapper<decltype(gradient_3D)> norm_wrapper (gradient_3D);
        auto output = Image<float>::create (argument[2], norm_wrapper);
        norm_wrapper (gradient_3D, output);
      } else {
        // TODO Until Vector2Axis is removed and a template header is provided by the adapter,
        //   just do it manually here
      }
    }
*/
  }
}



/*

template <class FilterType>
void process1D (Image<float>& input, const bool magnitude, const bool wrt_scanner,
                const std::string& filter_name, const vector<default_type>& stdev, const std::string& output_path)
{
  FilterType filter (input, magnitude);
  filter.compute_wrt_scanner (wrt_scanner);
  filter.set_message (std::string("applying " + filter_name + " filter to image " + input.name()));
  Stride::set_from_command_line (filter);
  filter.set_stdev (stdev);
  auto output = Image<float>::create (output_path, filter);
  filter (input, output);
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
          output.value() = abs (cdouble(temp.value()));
      } else {
        auto output = Image<cdouble>::create (argument[2], filter);
        filter (input, output);
      }
      break;
    }


    // Gradient / Laplacian1
    case 1:
    case 3:
    {
      auto input = Image<float>::open (argument[0]);

      vector<default_type> stdev;
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
          stdev[dim] = input.spacing (dim);
      }

      if (filter_index == 1)
        process1D<Filter::Gradient> (input, get_options ("magnitude").size(), get_options ("scanner").size(),
                                     "gradient", stdev, argument[2]);
      else
        process1D<Filter::Laplacian> (input, get_options ("magnitude").size(), get_options ("scanner").size(),
                                      "Laplacian", stdev, argument[2]);

      break;
    }


    // Identity
    case 2:
    {
      auto input = Image<float>::open (argument[0]);
      auto edge_handler = Adapter::EdgeExtend<Image<float>> (input);
      Adapter::Kernel::Single<decltype(edge_handler)> adapter (edge_handler, Filter::Kernels::identity);
      auto output = Image<float>::create (argument[2], adapter);
      copy (adapter, output);
      break;
    }


    // Laplacian2
    case 4:
    {
      auto input = Image<float>::open (argument[0]);
      auto edge_handler = Adapter::EdgeExtend<Image<float>> (input);
      Adapter::Kernel::Single<decltype(edge_handler)> adapter (edge_handler, Filter::Kernels::laplacian);
      auto output = Image<float>::create (argument[2], adapter);
      copy (adapter, output);
      break;
    }


    // Median
    case 5:
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


    // Normalisation
    case 6:
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


    // Sharpen
    case 7:
    {
      auto input = Image<float>::open (argument[0]);
      auto edge_handler = Adapter::EdgeExtend<Image<float>> (input);
      Adapter::Kernel::Single<decltype(edge_handler)> adapter (edge_handler, Filter::Kernels::sharpen);
      auto output = Image<float>::create (argument[2], adapter);
      copy (adapter, output);
      break;
    }


    // Smooth
    case 8:
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
        vector<default_type> stdevs = parse_floats((opt[0][0]));
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
      threaded_copy (input, output);
      filter (output);
      break;
    }


    // Zclean
    case 9:
    {
      auto input = Image<float>::open (argument[0]);
      Filter::ZClean filter (input);

      auto opt = get_options ("maskin");
      if (!opt.size())
        throw Exception (std::string(argument[1]) + " filter requires initial mask");
      Image<float> maskin = Image<float>::open (opt[0][0]);
      check_dimensions (maskin, input, 0, 3);

      filter.set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]) + "...");
      Stride::set_from_command_line (filter);

      filter.set_voxels_to_bridge (get_option_value ("bridge", 4));
      float zlower = get_option_value ("zlower", 2.5);
      float zupper = get_option_value ("zupper", 2.5);
      filter.set_zlim (zlower, zupper);

      auto output = Image<float>::create (argument[2], filter);
      filter (input, maskin, output);

      opt = get_options ("maskout");
      if (opt.size()) {
        auto maskout = Image<bool>::create (opt[0][0], filter.mask);
        threaded_copy (filter.mask, maskout);
      }
      break;
    }

    default:
      assert (0);
      break;
  }
}
*/
