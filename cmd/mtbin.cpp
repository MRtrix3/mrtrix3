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



#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "adapter/extract.h"
#include "filter/optimal_threshold.h"
#include "transform.h"
#include "math/least_squares.h"

using namespace MR;
using namespace App;

#define DEFAULT_NORM_VALUE 0.282094
#define DEFAULT_MAXITER_VALUE 100

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
   + "Multi-Tissue Bias field correction and Intensity Normalisation (MTBIN). This script inputs N number of tissue components "
     "(e.g. from multi-tissue CSD), and outputs N corrected tissue components. Intensity normalisation is performed by either normalising "
     "each tissue type independently with a single global scale factor per tissue or determining a normalise all tissues with the same scale "
     "factor (default). Example usage: mtbin wm.mif wm_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif. "
     "The estimated multiplicative bias field is guaranteed to have a mean of 1 over all voxels within the mask";

  ARGUMENTS
    + Argument ("input output", "list of all input and output tissue compartment files. See example usage in the description. "
                              "Note that any number of tissues can be normalised").type_image_in().allow_multiple();

  OPTIONS
    + Option ("mask", "define the mask to compute the normalisation within. If not supplied this is estimated automatically")
    + Argument ("image").type_image_in ()

    + Option ("value", "specify the value to which the summed tissue compartments will be to "
                       "(Default: sqrt(1/(4*pi) = " + str(DEFAULT_NORM_VALUE, 3) + ")")
    + Argument ("number").type_float ()

    + Option ("bias", "output the estimated bias field")
    + Argument ("image").type_image_out ()

    + Option ("independent", "intensity normalise each tissue type independently")

    + Option ("maxiter", "set the maximum number of iterations. Default(" + str(DEFAULT_MAXITER_VALUE) + "). "
                         "It will stop before the max iterations if convergence is detected")
    + Argument ("number").type_integer();
}


Eigen::MatrixXd basis_function (Eigen::Vector3 pos) {
    float x = (float)pos[0];
    float y = (float)pos[1];
    float z = (float)pos[2];
    Eigen::MatrixXd basis(19, 1);
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
    return basis;
}




void run ()
{
  if (argument.size() % 2)
    throw Exception ("The number of input arguments must be even. There must be an output file provided for every input tissue image");

  if (argument.size() < 4)
    throw Exception ("At least two tissue types must be provided");

  std::vector<Image<float> > input_images;
  std::vector<Header> output_headers;
  std::vector<std::string> output_filenames;


  // Load only first 3D volume of input images
  for (size_t i = 0; i < argument.size(); i += 2) {
    Header header = Header::open (argument[i]);
    input_images.emplace_back (Image<float>::open (argument[i]));

    // check if all inputs have the same dimensions
    if (i)
      check_dimensions (input_images[0], input_images[i / 2], 0, 3);

    // TODO check for output and not -force

    // we can't create the image yet if we want to put the scale factor into the output header
    output_filenames.push_back (argument[i + 1]);
    output_headers.emplace_back (header);
  }


  // Load or compute a mask to work with
  Image<bool> mask;
  Header header_3D (input_images[0]);
  header_3D.ndim() = 3;
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
  } else {
    auto summed = Image<float>::scratch (header_3D, "summed tissue compartment image");
    for (size_t j = 0; j < input_images.size(); ++j) {
      for (auto i = Loop (summed, 0, 3) (summed, input_images[j]); i; ++i)
        summed.value() += input_images[j].value();
    }
    Filter::OptimalThreshold threshold_filter (summed);
    mask = Image<bool>::scratch (threshold_filter);
    threshold_filter (summed, mask);
  }
  size_t num_voxels = 0;
  for (auto i = Loop (mask) (mask); i; ++i) {
    if (mask.value())
      num_voxels++;
  }

  // TODO reject outliers


  if (!num_voxels)
    throw Exception ("error in automatic mask generation. Mask contains no voxels");

  const float normalisation_value = get_option_value ("value", DEFAULT_NORM_VALUE);

  // Initialise bias field to 1
  Eigen::MatrixXd bias_field_weights (19, 1);
  bias_field_weights.setZero();
  bias_field_weights(0,0) = 1.0;

  // Pre-compute bias field basis functions for all voxels
  Transform transform (mask);
  uint32_t index = 0;
  Eigen::MatrixXd bias_field_basis (num_voxels, 19);
  for (auto i = Loop(mask)(mask); i; ++i){
    if (mask.value()) {
      Eigen::Vector3 vox (mask.index(0), mask.index(1), mask.index(2));
      Eigen::Vector3 pos = transform.voxel2scanner * vox;
      bias_field_basis.row (index++) = basis_function (pos).col(0);
    }
  }

  auto bias_field = Image<float>::scratch (header_3D);
  for (auto i = Loop(bias_field)(bias_field); i; ++i)
    bias_field.value() = 1.0;

  const size_t max_iter = get_option_value ("maxiter", DEFAULT_MAXITER_VALUE);

  // Iterate until convergence or max iterations performed
  Eigen::MatrixXd scale_factors (input_images.size(), 1);
  Eigen::MatrixXd previous_scale_factors (input_images.size(), 1);

  {
    ProgressBar progress ("normalising tissue compartments and estimating biasfield...");

    // TODO check for convergence
    size_t iter = 0;
    bool converged = false;
    while (!converged && iter < max_iter) {
      Eigen::MatrixXd X (num_voxels, input_images.size());
      Eigen::MatrixXd y (num_voxels, 1);
      y.fill (normalisation_value);
      index = 0;
      for (auto i = Loop (mask) (mask, bias_field); i; ++i) {
        if (mask.value()) {
//          float bias_field_value = bias_field_basis.row (index).dot (bias_field_weights.col(0));
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

      index = 0;
      for (auto i = Loop (mask) (mask); i; ++i) {
        if (mask.value()) {
          double sum = 0.0;
          for (size_t j = 0; j < input_images.size(); ++j) {
            assign_pos_of (mask, 0, 3).to (input_images[j]);
            sum += scale_factors(j,0) * input_images[j].value() ;
          }
          y(index++, 0) = sum / normalisation_value;
        }
      }
      progress++;

      bias_field_weights = bias_field_basis.colPivHouseholderQr().solve(y);
      //bias_field_weights(0,0) = 1.0;  // TODO normalise whole field or only within mask?

      progress++;

      // Normalise the bias field within the mask
      double mean = 0.0;
      for (auto i = Loop (bias_field) (bias_field, mask); i; ++i) {
        if (mask.value()) {
          Eigen::Vector3 vox (bias_field.index(0), bias_field.index(1), bias_field.index(2));
          Eigen::Vector3 pos = transform.voxel2scanner * vox;
          bias_field.value() = basis_function (pos).col(0).dot (bias_field_weights.col(0));
          mean += bias_field.value();
        }
      }
      progress++;
      mean /= num_voxels;
      for (auto i = Loop (bias_field) (bias_field, mask); i; ++i) {
        if (mask.value())
          bias_field.value() /= mean;
      }
      iter++;

      if (iter) {
        Eigen::MatrixXd diff = (previous_scale_factors.array() - scale_factors.array());
        for (size_t i = 0; i < diff.rows(); ++i)
          diff(i,0) = std::abs(diff(i,0));
        std::cout << "diff" << diff << std::endl;
      }
      previous_scale_factors = scale_factors;
//      float percent_change = diff.col(0).sum() / previous_scale_factors.col(0).sum();
//      std::cout << "prevous" << previous_scale_factors << std::endl;
//      std::cout << "percent change " << percent_change << std::endl;

      progress++;
    }
  }


  // Construct entire bias field (including outside the mask)
  Image<float> bias_field_output;
  opt = get_options("bias");
  if (opt.size())
    bias_field_output = Image<float>::create (opt[0][0], header_3D);
  else
    bias_field_output = Image<float>::scratch (header_3D);

  for (auto i = Loop (bias_field_output) (bias_field_output); i; ++i) {
    Eigen::Vector3 vox (bias_field_output.index(0), bias_field_output.index(1), bias_field_output.index(2));
    Eigen::Vector3 pos = transform.voxel2scanner * vox;
    bias_field_output.value() = basis_function (pos).col(0).dot (bias_field_weights.col(0));
  }


  // compute mean of all scale factors in the log domain
  opt = get_options("independent");
  if (!opt.size()) {
    float mean = 0.0;
    for (int i = 0; i < scale_factors.size(); ++i)
      mean += std::log(scale_factors(i, 0));
    mean /= scale_factors.size();
    mean = std::exp (mean);
    scale_factors.fill (mean);
  }


  // output bias corrected and normalised tissue maps
  ProgressBar progress ("writing output files", input_images.size());
  for (size_t j = 0; j < output_filenames.size(); ++j) {
    output_headers[j].keyval()["normalisation_scale_factor"] = str(scale_factors(j, 0));
    auto output_image = Image<float>::create (output_filenames[j], output_headers[j]);
    for (auto i = Loop (output_image) (output_image, input_images[j]); i; ++i) {
      assign_pos_of (output_image, 0, 3).to (bias_field_output);
      output_image.value() = scale_factors(j, 0) * input_images[j].value() / bias_field_output.value();
    }
    progress++;
  }
}
