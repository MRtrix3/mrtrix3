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


#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "transform.h"
#include "math/least_squares.h"
#include "algo/threaded_copy.h"
#include "adapter/replicate.h"

using namespace MR;
using namespace App;

#define DEFAULT_NORM_VALUE 0.28209479177
#define DEFAULT_MAIN_ITER_VALUE 15
#define DEFAULT_BALANCE_MAXITER_VALUE 7
#define DEFAULT_POLY_ORDER 3

const char* poly_order_choices[] = { "0", "1", "2", "3", nullptr };

void usage ()
{
  AUTHOR = "Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Multi-tissue informed log-domain intensity normalisation";

  DESCRIPTION
   + "This command inputs any number of tissue components (e.g. from multi-tissue CSD) "
     "and outputs corresponding normalised tissue components. Intensity normalisation is "
     "performed in the log-domain, and can smoothly vary spatially to accomodate the "
     "effects of (residual) intensity inhomogeneities."

   + "The -mask option is mandatory and is optimally provided with a brain mask "
     "(such as the one obtained from dwi2mask earlier in the processing pipeline). "
     "Outlier areas with exceptionally low or high combined tissue contributions are "
     "accounted for and reoptimised as the intensity inhomogeneity estimation becomes "
     "more accurate."

   + "Example usage: mtnormalise wmfod.mif wmfod_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif -mask mask.mif.";

  ARGUMENTS
    + Argument ("input output", "list of all input and output tissue compartment files. See example usage in the description.").type_image_in().allow_multiple();

  OPTIONS
    + Option ("mask", "the mask defines the data used to compute the intensity normalisation. This option is mandatory.").required ()
    + Argument ("image").type_image_in ()

    + Option ("order", "the maximum order of the polynomial basis used to fit the normalisation field in the log-domain. An order of 0 is equivalent to not allowing spatial variance of the intensity normalisation factor. (default: " + str(DEFAULT_POLY_ORDER) + ")")
    + Argument ("number").type_choice (poly_order_choices)

    + Option ("niter", "set the number of iterations. (default: " + str(DEFAULT_MAIN_ITER_VALUE) + ")")
    + Argument ("number").type_integer()

    + Option ("check_norm", "output the final estimated spatially varying intensity level that is used for normalisation.")
    + Argument ("image").type_image_out ()

    + Option ("check_mask", "output the final mask used to compute the normalisation. "
                            "This mask excludes regions identified as outliers by the optimisation process.")
    + Argument ("image").type_image_out ()

    + Option ("value", "specify the (positive) reference value to which the summed tissue compartments will be normalised. "
                       "(default: " + str(DEFAULT_NORM_VALUE, 6) + ", SH DC term for unit angular integral)")
    + Argument ("number").type_float (std::numeric_limits<default_type>::min());

}


template <int poly_order>
struct PolyBasisFunction { MEMALIGN (PolyBasisFunction)

  const int n_basis_vecs = 20;

  FORCE_INLINE Eigen::MatrixXd operator () (const Eigen::Vector3& pos) {
    double x = pos[0];
    double y = pos[1];
    double z = pos[2];
    Eigen::MatrixXd basis(n_basis_vecs, 1);
    basis(0) = 1.0;
    basis(1) = x;
    basis(2) = y;
    basis(3) = z;
    basis(4) = x * x;
    basis(5) = y * y;
    basis(6) = z * z;
    basis(7) = x * y;
    basis(8) = x * z;
    basis(9) = y * z;
    basis(10) = x * x * x;
    basis(11) = y * y * y;
    basis(12) = z * z * z;
    basis(13) = x * x * y;
    basis(14) = x * x * z;
    basis(15) = y * y * x;
    basis(16) = y * y * z;
    basis(17) = z * z * x;
    basis(18) = z * z * y;
    basis(19) = x * y * z;
    return basis;
  }
};


template <>
struct PolyBasisFunction<0> { MEMALIGN (PolyBasisFunction<0>)
  const int n_basis_vecs = 1;

  PolyBasisFunction() {}
  FORCE_INLINE Eigen::MatrixXd operator () (const Eigen::Vector3&) {
    Eigen::MatrixXd basis(n_basis_vecs, 1);
    basis(0) = 1.0;
    return basis;
  }
};


template <>
struct PolyBasisFunction<1> { MEMALIGN (PolyBasisFunction<1>)
  const int n_basis_vecs = 4;

  FORCE_INLINE Eigen::MatrixXd operator () (const Eigen::Vector3& pos) {
    double x = pos[0];
    double y = pos[1];
    double z = pos[2];
    Eigen::MatrixXd basis(n_basis_vecs, 1);
    basis(0) = 1.0;
    basis(1) = x;
    basis(2) = y;
    basis(3) = z;
    return basis;
  }
};


template <>
struct PolyBasisFunction<2> { MEMALIGN (PolyBasisFunction<2>)
  const int n_basis_vecs = 10;

  FORCE_INLINE Eigen::MatrixXd operator () (const Eigen::Vector3& pos) {
    double x = pos[0];
    double y = pos[1];
    double z = pos[2];
    Eigen::MatrixXd basis(n_basis_vecs, 1);
    basis(0) = 1.0;
    basis(1) = x;
    basis(2) = y;
    basis(3) = z;
    basis(4) = x * x;
    basis(5) = y * y;
    basis(6) = z * z;
    basis(7) = x * y;
    basis(8) = x * z;
    basis(9) = y * z;
    return basis;
  }
};


// Removes non-physical voxels from the mask
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


template <int poly_order> void run_primitive ();

void run ()
{
  if (argument.size() % 2)
    throw Exception ("The number of arguments must be even, provided as pairs of each input and its corresponding output file.");

  // Get poly-order of basis function
  auto opt = get_options ("order");
  if (opt.size ()) {
    const int order = int(opt[0][0]);
    switch (order) {
      case 0:
        run_primitive<0> ();
        break;
      case 1:
        run_primitive<1> ();
        break;
      case 2:
        run_primitive<2> ();
        break;
      default:
        run_primitive<DEFAULT_POLY_ORDER> ();
        break;
    }
  } else
      run_primitive<DEFAULT_POLY_ORDER> ();
}


template <int poly_order>
void run_primitive () {

  PolyBasisFunction<poly_order> basis_function;

  using ImageType = Image<float>;
  using MaskType = Image<bool>;

  vector<Adapter::Replicate<ImageType>> input_images;
  vector<Header> output_headers;
  vector<std::string> output_filenames;

  ProgressBar input_progress ("loading input images");

  // Open input images and prepare output image headers
  for (size_t i = 0; i < argument.size(); i += 2) {
    input_progress++;

    auto image = ImageType::open (argument[i]);

    if (image.ndim () > 4)
      throw Exception ("Input image \"" + image.name() + "\" contains more than 4 dimensions.");

    // Elevate image dimensions to ensure it is 4-dimensional
    // e.g. x,y,z -> x,y,z,1
    // This ensures consistency across multiple tissue input images
    Header h_image4d (image);
    h_image4d.ndim() = 4;

    input_images.emplace_back (image, h_image4d);

    if (i > 0)
      check_dimensions (input_images[0], input_images[i / 2], 0, 3);

    if (Path::exists (argument[i + 1]) && !App::overwrite_files)
      throw Exception ("Output file \"" + argument[i] + "\" already exists. (use -force option to force overwrite)");

    output_headers.push_back (std::move (h_image4d));
    output_filenames.push_back (argument[i + 1]);
  }

  const size_t n_tissue_types = input_images.size();

  // Load the mask and refine the initial mask to exclude non-positive summed tissue components
  Header header_3D (input_images[0]);
  header_3D.ndim() = 3;
  auto opt = get_options ("mask");

  auto orig_mask = MaskType::open (opt[0][0]);
  auto initial_mask = MaskType::scratch (orig_mask, "Initial processing mask");
  auto mask = MaskType::scratch (orig_mask, "Processing mask");
  auto prev_mask = MaskType::scratch (orig_mask, "Previous processing mask");

  {
    auto summed = ImageType::scratch (header_3D, "Summed tissue volumes");
    for (size_t j = 0; j < input_images.size(); ++j) {
      input_progress++;

      for (auto i = Loop (0, 3) (summed, input_images[j]); i; ++i)
        summed.value() += input_images[j].value();
    }
    refine_mask (summed, orig_mask, initial_mask);
  }

  threaded_copy (initial_mask, mask);


  // Load input images into single 4d-image and zero-clamp combined-tissue image
  Header h_combined_tissue (input_images[0]);
  h_combined_tissue.ndim () = 4;
  h_combined_tissue.size (3) = n_tissue_types;
  auto combined_tissue = ImageType::scratch (h_combined_tissue, "Tissue components");

  for (size_t i = 0; i < n_tissue_types; ++i) {
    input_progress++;

    combined_tissue.index (3) = i;
    for (auto l = Loop (0, 3) (combined_tissue, input_images[i]); l; ++l) {
      combined_tissue.value () = std::max<float>(input_images[i].value (), 0.f);
    }
  }

  size_t num_voxels = 0;
  for (auto i = Loop (mask) (mask); i; ++i) {
    if (mask.value())
      num_voxels++;
  }

  if (!num_voxels)
    throw Exception ("Mask contains no valid voxels.");


  const float normalisation_value = get_option_value ("value", DEFAULT_NORM_VALUE);
  const float log_norm_value = std::log (normalisation_value);
  const size_t max_iter = get_option_value ("niter", DEFAULT_MAIN_ITER_VALUE);
  const size_t max_balance_iter = DEFAULT_BALANCE_MAXITER_VALUE;

  // Initialise normalisation fields in both image and log domain
  Eigen::MatrixXd norm_field_weights;

  auto norm_field_image = ImageType::scratch (header_3D, "Normalisation field (intensity)");
  auto norm_field_log = ImageType::scratch (header_3D, "Normalisation field (log-domain)");

  for (auto i = Loop(norm_field_log) (norm_field_image, norm_field_log); i; ++i) {
    norm_field_image.value() = 1.f;
    norm_field_log.value() = 0.f;
  }

  Eigen::VectorXd balance_factors (Eigen::VectorXd::Ones (n_tissue_types));

  size_t iter = 1;

  // Store lambda-function for performing outlier-rejection.
  // We perform a coarse outlier-rejection initially as well as
  // a finer outlier-rejection within each iteration of the
  // tissue (re)balancing loop
  auto outlier_rejection = [&](float outlier_range) {

    auto summed_log = ImageType::scratch (header_3D, "Log of summed tissue volumes");
    for (auto i = Loop (0, 3) (summed_log, combined_tissue, norm_field_image); i; ++i) {
      for (size_t j = 0; j < n_tissue_types; ++j) {
        combined_tissue.index(3) = j;
        summed_log.value() += balance_factors(j) * combined_tissue.value() / norm_field_image.value();
      }
      summed_log.value() = std::log(summed_log.value());
    }

    threaded_copy (initial_mask, mask);

    vector<float> summed_log_values;
    summed_log_values.reserve (num_voxels);
    for (auto i = Loop (0, 3) (mask, summed_log); i; ++i) {
      if (mask.value())
        summed_log_values.push_back (summed_log.value());
    }

    num_voxels = summed_log_values.size();

    const auto lower_quartile_it = summed_log_values.begin() + std::round ((float)num_voxels * 0.25f);
    std::nth_element (summed_log_values.begin(), lower_quartile_it, summed_log_values.end());
    const float lower_quartile = *lower_quartile_it;
    const auto upper_quartile_it = summed_log_values.begin() + std::round ((float)num_voxels * 0.75f);
    std::nth_element (lower_quartile_it, upper_quartile_it, summed_log_values.end());
    const float upper_quartile = *upper_quartile_it;
    const float lower_outlier_threshold = lower_quartile - outlier_range * (upper_quartile - lower_quartile);
    const float upper_outlier_threshold = upper_quartile + outlier_range * (upper_quartile - lower_quartile);

    for (auto i = Loop (0, 3) (mask, summed_log); i; ++i) {
      if (mask.value()) {
        if (summed_log.value() < lower_outlier_threshold || summed_log.value() > upper_outlier_threshold) {
          mask.value() = 0;
          num_voxels--;
        }
      }
    }

    if (log_level >= 3)
      display (mask);
  };

  input_progress.done ();
  ProgressBar progress ("performing log-domain intensity normalisation", max_iter);

  // Perform an initial outlier rejection prior to the first iteration
  outlier_rejection (3.f);

  threaded_copy (mask, prev_mask);

  while (iter <= max_iter) {

    INFO ("Iteration: " + str(iter));

    // Iteratively compute tissue balance factors with outlier rejection
    size_t balance_iter = 1;
    bool balance_converged = false;

    while (!balance_converged && balance_iter <= max_balance_iter) {

      DEBUG ("Balance and outlier rejection iteration " + str(balance_iter) + " starts.");

      if (n_tissue_types > 1) {

        // Solve for tissue balance factors
        Eigen::MatrixXd X (num_voxels, n_tissue_types);
        Eigen::VectorXd y (Eigen::VectorXd::Ones (num_voxels));
        uint32_t index = 0;

        for (auto i = Loop (0, 3) (mask, combined_tissue, norm_field_image); i; ++i) {
          if (mask.value()) {
            for (size_t j = 0; j < n_tissue_types; ++j) {
              combined_tissue.index (3) = j;
              X (index, j) = combined_tissue.value() / norm_field_image.value();
            }
            ++index;
          }
        }

        balance_factors = X.colPivHouseholderQr().solve(y);

        // Ensure our balance factors satisfy the condition that sum(log(balance_factors)) = 0
        double log_sum = 0.0;
        for (size_t j = 0; j < n_tissue_types; ++j) {
          if (balance_factors(j) <= 0.0)
            throw Exception ("Non-positive tissue balance factor was computed."
                             " Tissue index: " + str(j+1) + " Balance factor: " + str(balance_factors(j)) +
                             " Needs to be strictly positive!");
          log_sum += std::log (balance_factors(j));
        }
        balance_factors /= std::exp (log_sum / n_tissue_types);
      }

      INFO ("Balance factors (" + str(balance_iter) + "): " + str(balance_factors.transpose()));

      // Perform outlier rejection on log-domain of summed images
      outlier_rejection(1.5f);

      // Check for convergence
      balance_converged = true;
      for (auto i = Loop (0, 3) (mask, prev_mask); i; ++i) {
        if (mask.value() != prev_mask.value()) {
          balance_converged = false;
          break;
        }
      }

      threaded_copy (mask, prev_mask);

      balance_iter++;
    }


    // Solve for normalisation field weights in the log domain
    Transform transform (mask);
    Eigen::MatrixXd norm_field_basis (num_voxels, basis_function.n_basis_vecs);
    Eigen::VectorXd y (num_voxels);
    uint32_t index = 0;
    for (auto i = Loop (0, 3) (mask, combined_tissue); i; ++i) {
      if (mask.value()) {
        Eigen::Vector3 vox (mask.index(0), mask.index(1), mask.index(2));
        Eigen::Vector3 pos = transform.voxel2scanner * vox;
        norm_field_basis.row (index) = basis_function (pos).col(0);

        double sum = 0.0;
        for (size_t j = 0; j < n_tissue_types; ++j) {
          combined_tissue.index(3) = j;
          sum += balance_factors(j) * combined_tissue.value() ;
        }
        y (index++) = std::log(sum) - log_norm_value;
      }
    }

    norm_field_weights = norm_field_basis.colPivHouseholderQr().solve(y);

    // Generate normalisation field in the log domain
    for (auto i = Loop (0, 3) (norm_field_log); i; ++i) {
      Eigen::Vector3 vox (norm_field_log.index(0), norm_field_log.index(1), norm_field_log.index(2));
      Eigen::Vector3 pos = transform.voxel2scanner * vox;
      norm_field_log.value() = basis_function (pos).col(0).dot (norm_field_weights.col(0));
    }

    // Generate normalisation field in the image domain
    for (auto i = Loop (0, 3) (norm_field_log, norm_field_image); i; ++i)
      norm_field_image.value () = std::exp(norm_field_log.value());

    progress++;
    iter++;
  }

  progress.done();

  ProgressBar output_progress("writing output images");

  opt = get_options ("check_norm");
  if (opt.size()) {
    auto norm_field_output = ImageType::create (opt[0][0], header_3D);
    threaded_copy (norm_field_image, norm_field_output);
  }

  opt = get_options ("check_mask");
  if (opt.size()) {
    auto mask_output = ImageType::create (opt[0][0], mask);
    threaded_copy (mask, mask_output);
  }

  // Compute log-norm scale parameter (geometric mean of normalisation field in outlier-free mask).
  float lognorm_scale (0.f);
  if (num_voxels) {
    for (auto i = Loop (0,3) (mask, norm_field_log); i; ++i) {
      if (mask.value ())
        lognorm_scale += norm_field_log.value ();
    }

    lognorm_scale = std::exp(lognorm_scale / (float)num_voxels);
  }


  for (size_t j = 0; j < output_filenames.size(); ++j) {
    output_progress++;

    output_headers[j].keyval()["lognorm_scale"] = str(lognorm_scale);
    auto output_image = ImageType::create (output_filenames[j], output_headers[j]);
    const size_t n_vols = input_images[j].size(3);
    const Eigen::VectorXf zero_vec = Eigen::VectorXf::Zero (n_vols);

    for (auto i = Loop (0,3) (output_image, input_images[j], norm_field_image); i; ++i) {
      input_images[j].index(3) = 0;

      if (input_images[j].value() < 0.f)
        output_image.row(3) = zero_vec;
      else
        output_image.row(3) = Eigen::VectorXf{input_images[j].row(3)} / norm_field_image.value();
    }
  }
}
