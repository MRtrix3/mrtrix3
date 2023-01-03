/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "command.h"
#include "exception.h"
#include "image.h"
#include "image_helpers.h"

#include "adapter/replicate.h"
#include "adapter/subset.h"
#include "algo/loop.h"
#include "filter/optimal_threshold.h"


using namespace MR;
using namespace App;


enum class operator_type { LT, LE, GE, GT, UNDEFINED };
const char* const operator_list[] = { "lt", "le", "ge", "gt", nullptr };


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Create bitwise image by thresholding image intensity";

  DESCRIPTION
  + "The threshold value to be applied can be determined in one of a number of ways:"

  + "- If no relevant command-line option is used, the command will automatically "
      "determine an optimal threshold;"

  + "- The -abs option provides the threshold value explicitly;"

  + "- The -percentile, -top and -bottom options enable more fine-grained control "
      "over how the threshold value is determined."

  + "The -mask option only influences those image values that contribute "
    "toward the determination of the threshold value; once the threshold is determined, "
    "it is applied to the entire image, irrespective of use of the -mask option. If you "
    "wish for the voxels outside of the specified mask to additionally be excluded from "
    "the output mask, this can be achieved by providing the -out_masked option."

  + "The four operators available through the \"-comparison\" option (\"lt\", \"le\", \"ge\" and \"gt\") "
    "correspond to \"less-than\" (<), \"less-than-or-equal\" (<=), \"greater-than-or-equal\" (>=) "
    "and \"greater-than\" (>). This offers fine-grained control over how the thresholding "
    "operation will behave in the presence of values equivalent to the threshold. "
    "By default, the command will select voxels with values greater than or equal to the "
    "determined threshold (\"ge\"); unless the -bottom option is used, in which case "
    "after a threshold is determined from the relevant lowest-valued image voxels, those "
    "voxels with values less than or equal to that threshold (\"le\") are selected. "
    "This provides more fine-grained control than the -invert option; the latter "
    "is provided for backwards compatibility, but is equivalent to selection of the "
    "opposite comparison within this selection."

  + "If no output image path is specified, the command will instead write to "
    "standard output the determined threshold value.";

  REFERENCES
    + "* If not using any explicit thresholding mechanism: \n"
    "Ridgway, G. R.; Omar, R.; Ourselin, S.; Hill, D. L.; Warren, J. D. & Fox, N. C. "
    "Issues with threshold masking in voxel-based morphometry of atrophied brains. "
    "NeuroImage, 2009, 44, 99-111";

  ARGUMENTS
  + Argument ("input", "the input image to be thresholded").type_image_in()
  + Argument ("output", "the (optional) output binary image mask").optional().type_image_out();


  OPTIONS

  + OptionGroup ("Threshold determination mechanisms")

  + Option ("abs", "specify threshold value as absolute intensity")
    + Argument ("value").type_float()

  + Option ("percentile", "determine threshold based on some percentile of the image intensity distribution")
    + Argument ("value").type_float (0.0, 100.0)

  + Option ("top", "determine threshold that will result in selection of some number of top-valued voxels")
    + Argument ("count").type_integer (1)

  + Option ("bottom", "determine & apply threshold resulting in selection of some number of bottom-valued voxels "
                      "(note: implies threshold application operator of \"le\" unless otherwise specified)")
    + Argument ("count").type_integer (1)



  + OptionGroup ("Threshold determination modifiers")

  + Option ("allvolumes", "compute a single threshold for all image volumes, rather than an individual threshold per volume")

  + Option ("ignorezero", "ignore zero-valued input values during threshold determination")

  + Option ("mask", "compute the threshold based only on values within an input mask image")
    + Argument ("image").type_image_in ()



  + OptionGroup ("Threshold application modifiers")

  + Option ("comparison", "comparison operator to use when applying the threshold; "
                        "options are: " + join(operator_list, ",") + " (default = \"le\" for -bottom; \"ge\" otherwise)")
    + Argument ("choice").type_choice (operator_list)

  + Option ("invert", "invert the output binary mask "
                      "(equivalent to flipping the operator; provided for backwards compatibility)")

  + Option ("out_masked", "mask the output image based on the provided input mask image")

  + Option ("nan", "set voxels that fail the threshold to NaN rather than zero "
                   "(output image will be floating-point rather than binary)");

}


using value_type = float;



bool issue_degeneracy_warning = false;



Image<bool> get_mask (const Image<value_type>& in)
{
  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (in, mask, 0, 3);
    for (size_t axis = 3; axis != mask.ndim(); ++axis) {
      if (mask.size (axis) > 1 && axis < in.ndim() && mask.size (axis) != in.size (axis))
        throw Exception ("Dimensions of mask image do not match those of main image");
    }
  }
  return mask;
}


vector<value_type> get_data (Image<value_type>& in,
                             Image<bool>& mask,
                             const size_t max_axis,
                             const bool ignore_zero)
{
  vector<value_type> data;
  data.reserve (voxel_count (in, 0, max_axis));
  if (mask.valid()) {
    Adapter::Replicate<Image<bool>> mask_replicate (mask, in);
    if (ignore_zero) {
      for (auto l = Loop(in, 0, max_axis) (in, mask_replicate); l; ++l) {
        if (mask_replicate.value() && !std::isnan (static_cast<value_type>(in.value())) && in.value() != 0.0f)
          data.push_back (in.value());
      }
    } else {
      for (auto l = Loop(in, 0, max_axis) (in, mask_replicate); l; ++l) {
        if (mask_replicate.value() && !std::isnan (static_cast<value_type>(in.value())))
          data.push_back (in.value());
      }
    }
  } else {
    if (ignore_zero) {
      for (auto l = Loop(in, 0, max_axis) (in); l; ++l) {
        if (!std::isnan (static_cast<value_type>(in.value())) && in.value() != 0.0f)
          data.push_back (in.value());
      }
    } else {
      for (auto l = Loop(in, 0, max_axis) (in); l; ++l) {
          if (!std::isnan (static_cast<value_type>(in.value())))
        data.push_back (in.value());
      }
    }
  }
  if (!data.size())
    throw Exception ("No valid input data found; unable to determine threshold");
  return data;
}



default_type calculate (Image<value_type>& in,
                        Image<bool>& mask,
                        const size_t max_axis,
                        const default_type abs,
                        const default_type percentile,
                        const ssize_t bottom,
                        const ssize_t top,
                        const bool ignore_zero)
{
  if (std::isfinite (abs)) {

    return abs;

  } else if (std::isfinite (percentile)) {

    auto data = get_data (in, mask, max_axis, ignore_zero);
    if (percentile == 100.0) {
      return default_type(*std::max_element (data.begin(), data.end()));
    } else if (percentile == 0.0) {
      return default_type(*std::min_element (data.begin(), data.end()));
    } else {
      const default_type interp_index = 0.01 * percentile * (data.size()-1);
      const size_t lower_index = size_t(std::floor (interp_index));
      const default_type mu = interp_index - default_type(lower_index);
      std::nth_element (data.begin(), data.begin() + lower_index, data.end());
      const default_type lower_value = default_type(data[lower_index]);
      std::nth_element (data.begin(), data.begin() + lower_index + 1, data.end());
      const default_type upper_value = default_type(data[lower_index + 1]);
      return (1.0-mu)*lower_value + mu*upper_value;
    }

  } else if (std::max (bottom, top) >= 0) {

    auto data = get_data (in, mask, max_axis, ignore_zero);
    const ssize_t index (bottom >= 0 ?
                         size_t(bottom) - 1 :
                         (ssize_t(data.size()) - ssize_t(top)));
    if (index < 0 || index >= ssize_t(data.size()))
      throw Exception ("Number of valid input image values (" + str(data.size()) + ") less than number of voxels requested via -" + (bottom >= 0 ? "bottom" : "top") + " option (" + str(bottom >= 0 ? bottom : top) + ")");
    std::nth_element (data.begin(), data.begin() + index, data.end());
    const value_type threshold_float = data[index];
    if (index) {
      std::nth_element (data.begin(), data.begin() + index - 1, data.end());
      if (data[index-1] == threshold_float)
        issue_degeneracy_warning = true;
    }
    if (index < ssize_t(data.size()) - 1) {
      std::nth_element (data.begin(), data.begin() + index + 1, data.end());
      if (data[index+1] == threshold_float)
        issue_degeneracy_warning = true;
    }
    return default_type(threshold_float);

  } else { // No explicit mechanism option: do automatic thresholding

    if (max_axis < in.ndim()) {

      // Need to extract just the current 3D volume
      vector<size_t> in_from (in.ndim()), in_size (in.ndim());
      size_t axis;
      for (axis = 0; axis != 3; ++axis) {
        in_from[axis] = 0;
        in_size[axis] = in.size (axis);
      }
      for (; axis != in.ndim(); ++axis) {
        in_from[axis] = in.index (axis);
        in_size[axis] = 1;
      }
      Adapter::Subset<Image<value_type>> in_subset (in, in_from, in_size);
      if (mask.valid()) {
        vector<size_t> mask_from (mask.ndim()), mask_size (mask.ndim());
        for (axis = 0; axis != 3; ++axis) {
          mask_from[axis] = 0;
          mask_size[axis] = mask.size (axis);
        }
        for (; axis != mask.ndim(); ++axis) {
          mask_from[axis] = mask.index (axis);
          mask_size[axis] = 1;
        }
        Adapter::Subset<Image<bool>> mask_subset (mask, mask_from, mask_size);
        Adapter::Replicate<decltype(mask_subset)> mask_replicate (mask_subset, in_subset);
        return Filter::estimate_optimal_threshold (in_subset, mask_replicate);
      } else {
        return Filter::estimate_optimal_threshold (in_subset);
      }

    } else if (mask.valid()) {
      Adapter::Replicate<Image<bool>> mask_replicate (mask, in);
      return Filter::estimate_optimal_threshold (in, mask_replicate);
    } else {
      return Filter::estimate_optimal_threshold (in);
    }

  }
}



template <typename T>
void apply (Image<value_type>& in,
            Image<bool>& mask,
            Image<T>& out,
            const size_t max_axis,
            const value_type threshold,
            const operator_type comp,
            const bool mask_out)
{
  const T true_value = std::is_floating_point<T>::value ? 1.0 : true;
  const T false_value = std::is_floating_point<T>::value ? NaN : false;

  std::function<bool(value_type, value_type)> func;
  switch (comp) {
    case operator_type::LT: func = [] (const value_type in, const value_type ref) { return in <  ref; }; break;
    case operator_type::LE: func = [] (const value_type in, const value_type ref) { return in <= ref; }; break;
    case operator_type::GE: func = [] (const value_type in, const value_type ref) { return in >= ref; }; break;
    case operator_type::GT: func = [] (const value_type in, const value_type ref) { return in >  ref; }; break;
    case operator_type::UNDEFINED: assert (0);
  }

  if (mask_out) {
    assert (mask.valid());
    for (auto l = Loop(in, 0, max_axis) (in, mask, out); l; ++l)
      out.value() = !std::isnan (static_cast<value_type>(in.value())) && mask.value() && func (in.value(), threshold) ? true_value : false_value;
  } else {
    for (auto l = Loop(in, 0, max_axis) (in, out); l; ++l)
      out.value() = !std::isnan (static_cast<value_type>(in.value())) && func (in.value(), threshold) ? true_value : false_value;
  }
}




// TODO Don't write directly to std::cout;
//   will get hidden by /r of progress bar
// Alternatively, withhold progress bar if writing to std::cout
template <typename T>
void execute (Image<value_type>& in,
              Image<bool>& mask,
              const std::string& out_path,
              const default_type abs,
              const default_type percentile,
              const ssize_t bottom,
              const ssize_t top,
              const bool ignore_zero,
              const bool all_volumes,
              const operator_type op,
              const bool mask_out)
{
  const bool to_cout = out_path.empty();
  Image<T> out;
  if (!to_cout) {
    Header header_out (in);
    header_out.datatype() = DataType::from<T>();
    header_out.datatype().set_byte_order_native();
    out = Image<T>::create (out_path, header_out);
  }

  // Branch based on whether or not we need to process each image volume individually
  if (in.ndim() > 3 && !all_volumes) {

    // Do one volume at a time
    // If writing to cout, also add a newline between each volume
    if (to_cout) {
      LogLevelLatch latch (App::log_level - 1);
      bool is_first_loop = true;
      for (auto l = Loop(3, in.ndim()) (in); l; ++l) {
        if (is_first_loop)
          is_first_loop = false;
        else
          std::cout << "\n";
        const default_type threshold = calculate (in, mask, 3, abs, percentile, bottom, top, ignore_zero);
        std::cout << threshold;
      }

    } else {

      for (auto l = Loop("Determining and applying per-volume thresholds", 3, in.ndim()) (in); l; ++l) {
        LogLevelLatch latch (App::log_level - 1);
        const default_type threshold = calculate (in, mask, 3, abs, percentile, bottom, top, ignore_zero);
        assign_pos_of (in, 3).to (out);
        apply (in, mask, out, 3, value_type(threshold), op, mask_out);
      }

    }

    return;

  } else if (in.ndim() <= 3 && all_volumes) {
    WARN ("Option -allvolumes ignored; input image is less than 4D");
  }

  // Process whole input image as a single block
  const default_type threshold = calculate (in, mask, in.ndim(), abs, percentile, bottom, top, ignore_zero);
  if (to_cout)
    std::cout << threshold;
  else
    apply (in, mask, out, in.ndim(), value_type(threshold), op, mask_out);

}





void run ()
{
  const default_type abs = get_option_value ("abs", NaN);
  const default_type percentile = get_option_value ("percentile", NaN);
  const ssize_t bottom = get_option_value ("bottom", -1);
  const ssize_t top = get_option_value ("top", -1);
  const size_t num_explicit_mechanisms = (std::isfinite (abs) ? 1 : 0) +
                                         (std::isfinite (percentile) ? 1 : 0) +
                                         (bottom >= 0 ? 1 : 0) +
                                         (top >= 0 ? 1 : 0);
  if (num_explicit_mechanisms > 1)
    throw Exception ("Cannot specify more than one mechanism for threshold selection");

  auto header_in = Header::open (argument[0]);
  if (header_in.datatype().is_complex())
    throw Exception ("Cannot perform thresholding directly on complex image data");
  auto in = header_in.get_image<value_type>();

  const bool to_cout = argument.size() == 1;
  const std::string output_path = to_cout ? std::string("") : argument[1];
  const bool all_volumes = get_options("allvolumes").size();
  const bool ignore_zero = get_options("ignorezero").size();
  const bool use_nan = get_options ("nan").size();
  const bool invert = get_options ("invert").size();

  bool mask_out = get_options ("out_masked").size();

  auto opt = get_options ("comparison");
  operator_type comp = opt.size() ?
                       operator_type(int(opt[0][0])) :
                       (bottom >= 0 ? operator_type::LE : operator_type::GE);
  if (invert) {
    switch (comp) {
      case operator_type::LT: comp = operator_type::GE; break;
      case operator_type::LE: comp = operator_type::GT; break;
      case operator_type::GE: comp = operator_type::LT; break;
      case operator_type::GT: comp = operator_type::LE; break;
      case operator_type::UNDEFINED: assert (0);
    }
  }

  if (to_cout) {
    if (use_nan) {
      WARN ("Option -nan ignored: has no influence when no output image is specified");
    }
    if (opt.size()) {
      WARN ("Option -comparison ignored: has no influence when no output image is specified");
      comp = operator_type::UNDEFINED;
    }
    if (invert) {
      WARN ("Option -invert ignored: has no influence when no output image is specified");
    }
    if (mask_out) {
      WARN ("Option -out_masked ignored: has no influence when no output image is specified");
    }
  }

  Image<bool> mask;
  if (std::isfinite (abs)) {
    if (ignore_zero) {
      WARN ("-ignorezero option has no effect if combined with -abs option");
    }
    if (get_options ("mask").size() && !mask_out) {
      WARN ("-mask option has no effect if combined with -abs option and -out_masked is not used");
    }
  } else {
    mask = get_mask (in);
    if (!mask.valid() && mask_out) {
      WARN ("-out_masked option ignored; no mask image provided via -mask");
      mask_out = false;
    }
    if (!num_explicit_mechanisms) {
      if (ignore_zero) {
        WARN ("Option -ignorezero ignored by automatic threshold calculation");
      }
      try {
        check_3D_nonunity (in);
      } catch (Exception& e) {
        throw Exception (e, "Automatic thresholding can only be performed for voxel data");
      }
    }
  }

  if (use_nan)
    execute<value_type> (in, mask, output_path, abs, percentile, bottom, top, ignore_zero, all_volumes, comp, mask_out);
  else
    execute<bool>       (in, mask, output_path, abs, percentile, bottom, top, ignore_zero, all_volumes, comp, mask_out);

  if (issue_degeneracy_warning) {
    WARN ("Duplicate image values surrounding threshold; "
          "exact number of voxels influenced by numerical threshold may not match requested number");
  }
}
