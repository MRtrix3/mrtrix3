/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <algorithm>
#include <cmath>

#include "command.h"
#include "datatype.h"
#include "header.h"
#include "image.h"

#include "algo/histogram.h"
#include "algo/loop.h"


using namespace MR;
using namespace App;

const char* choices[] = { "scale", "linear", "nonlinear", nullptr };

void usage () {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Modify the intensities of one image to match the histogram of another";

  ARGUMENTS
    + Argument ("type", "type of histogram matching to perform; options are: " + join(choices, ",")).type_choice (choices)
    + Argument ("input", "the input image to be modified").type_image_in ()
    + Argument ("target", "the input image from which to derive the target histogram").type_image_in()
    + Argument ("output", "the output image").type_image_out();

  OPTIONS
    + OptionGroup ("Image masking options")
    + Option ("mask_input", "only generate input histogram based on a specified binary mask image")
      + Argument ("image").type_image_in ()
    + Option ("mask_target", "only generate target histogram based on a specified binary mask image")
      + Argument ("image").type_image_in ()

    + OptionGroup ("Non-linear histogram matching options")
    + Option ("bins", "the number of bins to use to generate the histograms")
      + Argument ("num").type_integer (2);


  REFERENCES
    + "* If using inverse contrast normalization for inter-modal (DWI - T1) registration:\n"
      "Bhushan, C.; Haldar, J. P.; Choi, S.; Joshi, A. A.; Shattuck, D. W. & Leahy, R. M. "
      "Co-registration and distortion correction of diffusion and anatomical images based on inverse contrast normalization. "
      "NeuroImage, 2015, 115, 269-280";

}




void match_linear (Image<float>& input,
                   Image<float>& target,
                   Image<bool>& mask_input,
                   Image<bool>& mask_target,
                   const bool estimate_intercept)
{
  vector<float> input_data, target_data;
  {
    ProgressBar progress ("Loading & sorting image data", 4);
    if (mask_input.valid()) {
      for (auto l = Loop(input) (input, mask_input); l; ++l) {
        if (mask_input.value())
          input_data.push_back (input.value());
      }
    } else {
      for (auto l = Loop(input) (input); l; ++l)
        input_data.push_back (input.value());
    }
    ++progress;
    if (mask_target.valid()) {
      for (auto l = Loop(target) (target, mask_target); l; ++l) {
        if (mask_target.value())
          target_data.push_back (target.value());
      }
    } else {
      for (auto l = Loop(target) (target); l; ++l)
        target_data.push_back (target.value());
    }
    ++progress;
    std::sort (input_data.begin(), input_data.end());
    ++progress;
    std::sort (target_data.begin(), target_data.end());
  }

  // Ax=b
  // A: Input data
  // x: Model parameters; in the "scale" case, it's a single multiplier; if "linear", include a column of ones and estimate an intercept
  // b: Output data (or actually, interpolated histogram-matched output data)
  Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> input_matrix (input_data.size(), estimate_intercept ? 2 : 1);
  Eigen::Matrix<float, Eigen::Dynamic, 1> output_vector (input_data.size());
  for (size_t input_index = 0; input_index != input_data.size()-1; ++input_index) {
    input_matrix(input_index, 0) = input_data[input_index];
    const default_type output_position = (target_data.size()-1) * (default_type(input_index) / default_type(input_data.size()-1));
    const size_t target_index_lower = std::floor (output_position);
    const default_type mu = output_position - default_type(target_index_lower);
    output_vector[input_index] = ((1.0-mu)*target_data[target_index_lower] + mu*target_data[target_index_lower+1]);
  }
  input_matrix(input_data.size()-1, 0) = input_data.back();
  output_vector[input_data.size()-1] = target_data.back();
  if (estimate_intercept)
    input_matrix.col(1).fill (1.0f);

  auto parameters = (input_matrix.transpose() * input_matrix).ldlt().solve(input_matrix.transpose() * output_vector).eval();

  Header H (input);
  H.datatype() = DataType::Float32;
  H.datatype().set_byte_order_native();
  H.keyval()["mrhistmatch_scale"] = str<float>(parameters[0]);
  if (estimate_intercept) {
    CONSOLE ("Estimated linear transform is: " + str(parameters[0]) + "x + " + str(parameters[1]));
    H.keyval()["mrhistmatch_offset"] = str<float>(parameters[1]);
    auto output = Image<float>::create (argument[3], H);
    for (auto l = Loop("Writing output image data", input) (input, output); l; ++l) {
      if (std::isfinite(static_cast<float>(input.value()))) {
        output.value() = parameters[0]*input.value() + parameters[1];
      } else {
        output.value() = 0.0;
      }
    }
  } else {
    CONSOLE ("Estimated scale factor is " + str(parameters[0]));
    auto output = Image<float>::create (argument[3], H);
    for (auto l = Loop("Writing output image data", input) (input, output); l; ++l) {
      if (std::isfinite(static_cast<float>(input.value()))) {
        output.value() = input.value() * parameters[0];
      } else {
        output.value() = 0.0;
      }
    }
  }
}




void match_nonlinear (Image<float>& input,
                      Image<float>& target,
                      Image<bool>& mask_input,
                      Image<bool>& mask_target,
                      const size_t nbins)
{
  Algo::Histogram::Calibrator calib_input (nbins, true);
  Algo::Histogram::calibrate (calib_input, input, mask_input);
  INFO ("Input histogram ranges from " + str(calib_input.get_min()) + " to " + str(calib_input.get_max()) + "; using " + str(calib_input.get_num_bins()) + " bins");
  Algo::Histogram::Data hist_input = Algo::Histogram::generate (calib_input, input, mask_input);

  Algo::Histogram::Calibrator calib_target (nbins, true);
  Algo::Histogram::calibrate (calib_target, target, mask_target);
  INFO ("Target histogram ranges from " + str(calib_target.get_min()) + " to " + str(calib_target.get_max()) + "; using " + str(calib_target.get_num_bins()) + " bins");
  Algo::Histogram::Data hist_target = Algo::Histogram::generate (calib_target, target, mask_target);

  // Non-linear intensity mapping determined within this class
  Algo::Histogram::Matcher matcher (hist_input, hist_target);

  Header H (input);
  H.datatype() = DataType::Float32;
  H.datatype().set_byte_order_native();
  auto output = Image<float>::create (argument[3], H);
  for (auto l = Loop("Writing output data", input) (input, output); l; ++l) {
    if (std::isfinite(static_cast<float>(input.value()))) {
      output.value() = matcher (input.value());
    } else {
      output.value() = 0.0;
    }
  }
}





void run ()
{
  auto input  = Image<float>::open (argument[1]);
  auto target = Image<float>::open (argument[2]);
  if (input.ndim() > 3 || target.ndim() > 3)
    throw Exception ("mrhistmatch currently only works on 3D images");

  Image<bool> mask_input, mask_target;
  auto opt = get_options ("mask_input");
  if (opt.size()) {
    mask_input = Image<bool>::open (opt[0][0]);
    check_dimensions (input, mask_input);
  }
  opt = get_options ("mask_target");
  if (opt.size()) {
    mask_target = Image<bool>::open (opt[0][0]);
    check_dimensions (target, mask_target);
  }

  switch (int(argument[0])) {
    case 0: // Scale
      match_linear (input, target, mask_input, mask_target, false);
      break;
    case 1: // Linear
      match_linear (input, target, mask_input, mask_target, true);
      break;
    case 2: // Non-linear
      match_nonlinear (input, target, mask_input, mask_target, get_option_value ("bins", 0));
      break;
    default:
      throw Exception ("Undefined histogram matching type");
  }
}

