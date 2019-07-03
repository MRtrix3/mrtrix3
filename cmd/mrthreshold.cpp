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

#include "command.h"
#include "exception.h"
#include "image.h"
#include "image_helpers.h"

#include "algo/loop.h"
#include "filter/optimal_threshold.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)";

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
    "it is applied to the entire image, irrespective of use of the -mask option."

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
  + OptionGroup ("Different mechanisms for determining the threshold value (use no more than one)")

  + Option ("abs", "specify threshold value as absolute intensity")
    + Argument ("value").type_float()

  + Option ("percentile", "determine threshold based on some percentile of the image intensity distribution")
    + Argument ("value").type_float (0.0, 100.0)

  + Option ("top", "determine threshold that will result in selection of some number of top-valued voxels")
    + Argument ("count").type_integer (1)

  + Option ("bottom", "determine threshold that will result in omission of some number of bottom-valued voxels")
    + Argument ("count").type_integer (1)


  + OptionGroup ("Options that influence determination of the threshold based on the input image")

  + Option ("ignorezero", "ignore zero-valued input values")

  + Option ("mask", "compute the threshold based only on values within an input mask image")
    + Argument ("image").type_image_in ()


  + OptionGroup ("Options that influence generation of the output image after the threshold is determined")

  + Option ("invert", "invert the output binary mask")

  + Option ("nan", "set voxels that fail the threshold to NaN rather than zero.");

}


using value_type = float;


Image<bool> get_mask (const Image<value_type>& in)
{
  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (in, mask);
  }
  return mask;
}


vector<value_type> get_data (Image<value_type> in)
{
  const bool ignorezero = get_options("ignorezero").size();
  vector<value_type> data;
  Image<bool> mask = get_mask (in);
  if (mask.valid()) {
    if (ignorezero) {
      for (auto l = Loop(in) (in, mask); l; ++l) {
        if (mask.value() && in.value())
          data.push_back (in.value());
      }
    } else {
      for (auto l = Loop(in) (in, mask); l; ++l) {
        if (mask.value() && std::isfinite (in.value()))
          data.push_back (in.value());
      }
    }
  } else {
    if (ignorezero) {
      for (auto l = Loop(in) (in); l; ++l) {
        if (in.value())
          data.push_back (in.value());
      }
    } else {
      for (auto l = Loop(in) (in); l; ++l) {
          if (std::isfinite (in.value()))
        data.push_back (in.value());
      }
    }
  }
  if (!data.size())
    throw Exception ("No valid input data found; unable to determine threshold");
  return data;
}



template <typename T>
void save (Image<float> in,
           Header H,
           const default_type threshold,
           const bool invert,
           const bool equal_as_above,
           const std::string& path)
{
  H.datatype() = DataType::from<T>();
  T above = std::is_floating_point<T>::value ? 1.0 : true;
  T below = std::is_floating_point<T>::value ? NaN : false;
  const T nonfinite = below;
  if (invert)
    std::swap (above, below);
  Image<T> out = Image<T>::create (path, H);
  if (equal_as_above) {
    const value_type threshold_float (threshold);
    for (auto l = Loop(in) (in, out); l; ++l)
      out.value() = std::isfinite (in.value()) ?
                    ((in.value() >= threshold_float) ? above : below) :
                    nonfinite;
  } else {
    for (auto l = Loop(in) (in, out); l; ++l)
      out.value() = std::isfinite (in.value()) ?
                    ((default_type (in.value()) > threshold) ? above : below) :
                    nonfinite;
  }
}



void run ()
{
  const default_type abs = get_option_value ("abs", NaN);
  const default_type percentile = get_option_value ("percentile", NaN);
  const ssize_t bottom = get_option_value ("bottom", -1);
  const ssize_t top = get_option_value ("top", -1);
  if ((std::isfinite (abs) ? 1 : 0) +
      (std::isfinite (percentile) ? 1 : 0) +
      (bottom >= 0 ? 1 : 0) +
      (top >= 0 ? 1 : 0) > 1)
    throw Exception ("Cannot specify more than one mechanism for threshold selection");

  auto header = Header::open (argument[0]);
  if (header.datatype().is_complex())
    throw Exception ("Cannot perform thresholding on complex images");
  auto in = header.get_image<value_type>();

  const bool to_cout = argument.size() == 1;

  bool equal_as_above = true;
  default_type threshold;
  if (std::isfinite (abs)) {

    auto ignore_input_option = [&] (const std::string& opt) {
      if (get_options (opt).size()) {
        WARN ("Option -" + opt + " ignored: has no influence due to use of -abs option");
      }
    };
    ignore_input_option ("ignorezero");
    ignore_input_option ("mask");

    threshold = abs;

  } else if (get_options ("percentile").size()) {

    auto data = get_data (in);
    if (percentile == 100.0) {
      threshold = *std::max_element (data.begin(), data.end());
    } else if (!percentile) {
      threshold = *std::min_element (data.begin(), data.end());
    } else {
      const default_type interp_index = 0.01 * percentile * (data.size()-1);
      const size_t lower_index = std::floor (interp_index);
      const default_type mu = interp_index - default_type(lower_index);
      std::nth_element (data.begin(), data.begin() + lower_index, data.end());
      const value_type lower_value = data[lower_index];
      std::nth_element (data.begin(), data.begin() + lower_index + 1, data.end());
      const value_type upper_value = data[lower_index + 1];
      threshold = (1.0-mu)*lower_value + mu*upper_value;
    }

  } else if (std::max (bottom, top) >= 0) {

    auto data = get_data (in);
    const size_t index (bottom >= 0 ?
                        size_t(bottom) - 1 :
                        (data.size() - top));
    if (index > data.size())
      throw Exception ("Number of valid input image values (" + str(data.size()) + ") less than number of voxels requested via -" + (bottom >= 0 ? "bottom" : "top") + " option (" + str(index) + ")");
    std::nth_element (data.begin(), data.begin() + index, data.end());
    const value_type threshold_float = data[index];
    bool degeneracy_warning = false;
    if (index) {
      std::nth_element (data.begin(), data.begin() + index - 1, data.end());
      if (data[index-1] == threshold_float)
        degeneracy_warning = true;
    }
    if (index < data.size() - 1) {
      std::nth_element (data.begin(), data.begin() + index + 1, data.end());
      if (data[index+1] == threshold_float)
        degeneracy_warning = true;
    }
    std::sort (data.begin(), data.end());
    if (degeneracy_warning) {
      WARN ("Duplicate image values surrounding threshold; "
            "exact number of voxels influenced by numerical threshold may not match requested number");
    }
    threshold = default_type(threshold_float);
    // If threhsolding to remove some lower number of voxels, we want to
    //   _not_ accept any voxels for which the value is precisely equal to the threshold
    if (bottom >= 0)
      equal_as_above = false;

  } else { // No explicit mechanism option: do automatic thresholding

    if (get_options ("ignorezero").size()) {
      WARN ("Option -ignorezero ignored by automatic threshold calculation");
    }
    try {
      check_3D_nonunity (in);
    } catch (Exception& e) {
      throw Exception (e, "Automatic thresholding can only be performed for voxel data");
    }
    Image<bool> mask = get_mask (in);
    if (to_cout) {
      LogLevelLatch latch (App::log_level - 1);
      threshold = Filter::estimate_optimal_threshold (in, mask);
    } else {
      threshold = Filter::estimate_optimal_threshold (in, mask);
    }

  }

  if (to_cout) {
    auto ignore_output_option = [&] (const std::string& opt) {
      if (get_options (opt).size()) {
        WARN ("Option -" + opt + " ignored: has no influence when no output image is provided");
      }
    };
    ignore_output_option ("invert");
    ignore_output_option ("nan");
    std::cout << threshold;
    return;
  }

  const bool use_nan = get_options ("nan").size();
  const bool invert = get_options ("invert").size();
  if (use_nan)
    save<value_type> (in, header, threshold, invert, equal_as_above, argument[1]);
  else
    save<bool> (in, header, threshold, invert, equal_as_above, argument[1]);

}
