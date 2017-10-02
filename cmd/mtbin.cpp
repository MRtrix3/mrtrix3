/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "adapter/extract.h"
#include "filter/optimal_threshold.h"
#include "filter/mask_clean.h"
#include "filter/connected_components.h"
#include "transform.h"
#include "math/least_squares.h"
#include "algo/threaded_copy.h"

using namespace MR;
using namespace App;

#define DEFAULT_NORM_VALUE 0.282094
#define DEFAULT_MAXITER_VALUE 100

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au), Rami Tabbara (rami.tabbara@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)";

  SYNOPSIS = "Multi-Tissue Bias field correction and Intensity Normalisation (WARNING: deprecated).";

  DESCRIPTION
   + "WARNING: this command is deprecated and may produce highly inappropriate results in several cases. Not recommended and at your own discretion. Please use the new mtnormalise command instead for reliable results.";

  ARGUMENTS
    + Argument ("input output", "list of all input and output tissue compartment files. See example usage in the description. "
                              "Note that any number of tissues can be normalised").type_image_in().allow_multiple();

  OPTIONS
    + Option ("mask", "define the mask to compute the normalisation within. This option is mandatory.").required ()
    + Argument ("image").type_image_in ()

    + Option ("value", "specify the value to which the summed tissue compartments will be normalised to "
                       "(Default: sqrt(1/(4*pi)) = " + str(DEFAULT_NORM_VALUE, 6) + ")")
    + Argument ("number").type_float ()

    + Option ("bias", "output the estimated bias field")
    + Argument ("image").type_image_out ()

    + Option ("independent", "intensity normalise each tissue type independently")

    + Option ("maxiter", "set the maximum number of iterations. Default(" + str(DEFAULT_MAXITER_VALUE) + "). "
                         "It will stop before the max iterations if convergence is detected")
    + Argument ("number").type_integer()

    + Option ("check", "check the final mask used to compute the bias field. This mask excludes outlier regions ignored by the bias field fitting procedure. However, these regions are still corrected for bias fields based on the other image data.")
    + Argument ("image").type_image_out ()
    
    + Option ("override", "consciously use this deprecated command. Not recommended and at your own discretion.");
}

const int n_basis_vecs (20);


FORCE_INLINE Eigen::MatrixXd basis_function (const Eigen::Vector3 pos) {
  double x = pos[0];
  double y = pos[1];
  double z = pos[2];
  Eigen::MatrixXd basis(n_basis_vecs, 1);
  basis(0) = 1.0;
  basis(1) = x;
  basis(2) = y;
  basis(3) = z;
  basis(4) = x * y;
  basis(5) = x * z;
  basis(6) = y * z;
  basis(7) = x * x;
  basis(8) = y * y;
  basis(9)= z * x;
  basis(10)= x * x * y;
  basis(11) = x * x * z;
  basis(12) = y * y * x;
  basis(13) = y * y * z;
  basis(14) = z * z * x;
  basis(15) = z * z * y;
  basis(16) = x * x * x;
  basis(17) = y * y * y;
  basis(18) = z * z * z;
  basis(19) = x * y * z;
  return basis;
}

// Currently not used, but keep if we want to make mask argument optional in the future
FORCE_INLINE void compute_mask (Image<float>& summed, Image<bool>& mask) {
  LogLevelLatch level (0);
  Filter::OptimalThreshold threshold_filter (summed);
  if (!mask.valid())
    mask = Image<bool>::scratch (threshold_filter);
  threshold_filter (summed, mask);
  Filter::ConnectedComponents connected_filter (mask);
  connected_filter.set_largest_only (true);
  connected_filter (mask, mask);
  Filter::MaskClean clean_filter (mask);
  clean_filter (mask, mask);
}


FORCE_INLINE void refine_mask (Image<float>& summed,
  Image<bool>& initial_mask,
  Image<bool>& refined_mask) {

  for (auto i = Loop (summed, 0, 3) (summed, initial_mask, refined_mask); i; ++i) {
    if (std::isfinite((float) summed.value ()) && summed.value () > 0.f && initial_mask.value ())
      refined_mask.value () = true;
    else
      refined_mask.value () = false;
  }
}


void run ()
{

  WARN ("This command is deprecated and may produce inappropriate results in several cases. Please use the new mtnormalise.");
  
  if (!get_options("override").size())
    throw Exception ("This command is deprecated and not recommended for proper use of its original advertised functions. Check the option list for an option to consciously use this command anyway.");

  if (argument.size() % 2)
    throw Exception ("The number of input arguments must be even. There must be an output file provided for every input tissue image");

  if (argument.size() < 4)
    throw Exception ("At least two tissue types must be provided");

  ProgressBar progress ("performing intensity normalisation and bias field correction...");
  vector<Image<float> > input_images;
  vector<Header> output_headers;
  vector<std::string> output_filenames;


  // Open input images and check for output
  for (size_t i = 0; i < argument.size(); i += 2) {
    progress++;
    input_images.emplace_back (Image<float>::open (argument[i]));

    // check if all inputs have the same dimensions
    if (i)
      check_dimensions (input_images[0], input_images[i / 2], 0, 3);

    if (Path::exists (argument[i + 1]) && !App::overwrite_files)
      throw Exception ("output file \"" + argument[i] + "\" already exists (use -force option to force overwrite)");

    // we can't create the image yet if we want to put the scale factor into the output header
    output_headers.emplace_back (Header::open (argument[i]));
    output_filenames.push_back (argument[i + 1]);
  }

  // Load the mask
  Header header_3D (input_images[0]);
  header_3D.ndim() = 3;
  auto opt = get_options ("mask");

  auto orig_mask = Image<bool>::open (opt[0][0]);
  auto initial_mask = Image<bool>::scratch (orig_mask);
  auto mask = Image<bool>::scratch (orig_mask);

  auto summed = Image<float>::scratch (header_3D);
  for (size_t j = 0; j < input_images.size(); ++j) {
    for (auto i = Loop (summed, 0, 3) (summed, input_images[j]); i; ++i)
      summed.value() += input_images[j].value();
    progress++;
  }

  // Refine the initial mask to exclude negative summed tissue components
  refine_mask (summed, orig_mask, initial_mask);

  threaded_copy (initial_mask, mask);

  size_t num_voxels = 0;
  for (auto i = Loop (mask) (mask); i; ++i) {
    if (mask.value())
      num_voxels++;
  }
  progress++;

  if (!num_voxels)
    throw Exception ("error in automatic mask generation. Mask contains no voxels");

  const float normalisation_value = get_option_value ("value", DEFAULT_NORM_VALUE);
  const size_t max_iter = get_option_value ("maxiter", DEFAULT_MAXITER_VALUE);

  // Initialise bias field
  Eigen::MatrixXd bias_field_weights (n_basis_vecs, 1);
  auto bias_field = Image<float>::scratch (header_3D);
  for (auto i = Loop(bias_field)(bias_field); i; ++i)
    bias_field.value() = 1.0;

  Eigen::MatrixXd scale_factors (input_images.size(), 1);
  Eigen::MatrixXd previous_scale_factors (input_images.size(), 1);
  size_t iter = 1;
  bool converged = false;

  // Iterate until convergence or max iterations performed
  while (!converged && iter < max_iter) {

    INFO ("iteration: " + str(iter));
    // Solve for tissue normalisation scale factors
    Eigen::MatrixXd X (num_voxels, input_images.size());
    Eigen::MatrixXd y (num_voxels, 1);
    y.fill (normalisation_value);
    uint32_t index = 0;
    for (auto i = Loop (mask) (mask, bias_field); i; ++i) {
      if (mask.value()) {
        for (size_t j = 0; j < input_images.size(); ++j) {
          assign_pos_of (mask, 0, 3).to (input_images[j]);
          X (index, j) = input_images[j].value() / bias_field.value();
        }
        ++index;
      }
    }
    progress++;
    scale_factors = X.colPivHouseholderQr().solve(y);
    progress++;

    INFO ("scale factors: " + str(scale_factors.transpose()));


    // Solve for bias field weights
    Transform transform (mask);
    Eigen::MatrixXd bias_field_basis (num_voxels, n_basis_vecs);
    index = 0;
    for (auto i = Loop (mask) (mask); i; ++i) {
      if (mask.value()) {
        Eigen::Vector3 vox (mask.index(0), mask.index(1), mask.index(2));
        Eigen::Vector3 pos = transform.voxel2scanner * vox;
        bias_field_basis.row (index) = basis_function (pos).col(0);

        double sum = 0.0;
        for (size_t j = 0; j < input_images.size(); ++j) {
          assign_pos_of (mask, 0, 3).to (input_images[j]);
          sum += scale_factors(j,0) * input_images[j].value() ;
        }
        y (index++, 0) = sum / normalisation_value;
      }
    }
    progress++;
    bias_field_weights = bias_field_basis.colPivHouseholderQr().solve(y);
    progress++;


    // Normalise the bias field within the mask
    double mean = 0.0;
    for (auto i = Loop (bias_field) (bias_field, mask); i; ++i) {
        Eigen::Vector3 vox (bias_field.index(0), bias_field.index(1), bias_field.index(2));
        Eigen::Vector3 pos = transform.voxel2scanner * vox;
        bias_field.value() = basis_function (pos).col(0).dot (bias_field_weights.col(0));
        if (mask.value())
          mean += bias_field.value();
    }
    progress++;
    mean /= num_voxels;
    for (auto i = Loop (bias_field) (bias_field, mask); i; ++i)
      bias_field.value() /= mean;

    progress++;

    // Check for convergence
    Eigen::MatrixXd diff;
    if (iter > 1) {
      diff = previous_scale_factors.array() - scale_factors.array();
      diff = diff.array().abs() / previous_scale_factors.array();
      INFO ("percentage change in estimated scale factors: " + str(diff.mean() * 100));
      if (diff.mean() < 0.001)
        converged = true;
    }

    // Re-evaluate mask
    if (!converged) {
      auto summed = Image<float>::scratch (header_3D);
      for (size_t j = 0; j < input_images.size(); ++j) {
        for (auto i = Loop (summed, 0, 3) (summed, input_images[j], bias_field); i; ++i) {
            summed.value() += scale_factors(j, 0) * input_images[j].value() / bias_field.value();
        }
      }

      refine_mask (summed, initial_mask, mask);

      vector<float> summed_values;
      for (auto i = Loop (mask) (mask, summed); i; ++i) {
        if (mask.value())
          summed_values.push_back (summed.value());
      }
      num_voxels = summed_values.size();

      // Reject outliers after a few iterations once the summed image is largely corrected for the bias field
      if (iter > 2) {
        INFO ("rejecting outliers");
        std::sort (summed_values.begin(), summed_values.end());
        float lower_quartile = summed_values[std::round ((float)num_voxels * 0.25)];
        float upper_quartile = summed_values[std::round ((float)num_voxels * 0.75)];
        float upper_outlier_threshold = upper_quartile + 1.6 * (upper_quartile - lower_quartile);
        float lower_outlier_threshold = lower_quartile - 1.6 * (upper_quartile - lower_quartile);

        for (auto i = Loop (mask) (mask, summed); i; ++i) {
          if (mask.value()) {
            if (summed.value() < lower_outlier_threshold || summed.value() > upper_outlier_threshold) {
              mask.value() = 0;
              num_voxels--;
            }
          }
        }
      }
      if (log_level >= 3)
        display (mask);
    }

    previous_scale_factors = scale_factors;

    progress++;
    iter++;
  }

  opt = get_options ("bias");
  if (opt.size()) {
    auto bias_field_output = Image<float>::create (opt[0][0], header_3D);
    threaded_copy (bias_field, bias_field_output);
  }
  progress++;

  opt = get_options ("check");
  if (opt.size()) {
    auto mask_output = Image<float>::create (opt[0][0], mask);
    threaded_copy (mask, mask_output);
  }
  progress++;

  // compute mean of all scale factors in the log domain
  opt = get_options ("independent");
  if (!opt.size()) {
    float mean = 0.0;
    for (int i = 0; i < scale_factors.size(); ++i)
      mean += std::log(scale_factors(i, 0));
    mean /= scale_factors.size();
    mean = std::exp (mean);
    scale_factors.fill (mean);
  }

  // output bias corrected and normalised tissue maps
  uint32_t total_count = 0;
  for (size_t i = 0; i < output_headers.size(); ++i) {
    uint32_t count = 1;
    for (size_t j = 0; j < output_headers[i].ndim(); ++j)
      count *= output_headers[i].size(j);
    total_count += count;
  }
  for (size_t j = 0; j < output_filenames.size(); ++j) {
    output_headers[j].keyval()["normalisation_scale_factor"] = str(scale_factors(j, 0));
    auto output_image = Image<float>::create (output_filenames[j], output_headers[j]);
    for (auto i = Loop (output_image) (output_image, input_images[j]); i; ++i) {
      assign_pos_of (output_image, 0, 3).to (bias_field);
      output_image.value() = scale_factors(j, 0) * input_images[j].value() / bias_field.value();
    }
  }
  
  WARN ("Using the output images as part of a pipeline to analyse one's data is discouraged.");
  
}
