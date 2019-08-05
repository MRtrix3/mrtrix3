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
#include "image.h"
#include "algo/loop.h"
#include "transform.h"
#include "math/least_squares.h"
#include "algo/threaded_copy.h"
#include "adapter/replicate.h"

using namespace MR;
using namespace App;

#define DEFAULT_REFERENCE_VALUE 0.28209479177
#define DEFAULT_MAIN_ITER_VALUE 15
#define DEFAULT_BALANCE_MAXITER_VALUE 7
#define DEFAULT_POLY_ORDER 3

const char* poly_order_choices[] = { "0", "1", "2", "3", nullptr };

void usage ()
{
  AUTHOR = "Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Multi-tissue informed log-domain intensity normalisation";

  REFERENCES
    + "Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. " // Internal
    "Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. "
    "In Proc. ISMRM, 2017, 26, 3541";

  DESCRIPTION
   + "This command takes as input any number of tissue components (e.g. from "
     "multi-tissue CSD) and outputs corresponding normalised tissue components "
     "corrected for the effects of (residual) intensity inhomogeneities. "
     "Intensity normalisation is performed by optimising the voxel-wise sum of "
     "all tissue compartments towards a constant value, under constraints of "
     "spatial smoothness (polynomial basis of a given order). Different to "
     "the Raffelt et al. 2017 abstract, this algorithm performs this task "
     "in the log-domain instead, with added gradual outlier rejection, different "
     "handling of the balancing factors between tissue compartments and a "
     "different iteration structure."

   + "The -mask option is mandatory and is optimally provided with a brain mask "
     "(such as the one obtained from dwi2mask earlier in the processing pipeline). "
     "Outlier areas with exceptionally low or high combined tissue contributions are "
     "accounted for and reoptimised as the intensity inhomogeneity estimation becomes "
     "more accurate.";

  EXAMPLES
   + Example ("Default usage (for 3-tissue CSD compartments)",
              "mtnormalise wmfod.mif wmfod_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif -mask mask.mif",
              "Note how for each tissue compartment, the input and output images are provided as "
              "a consecutive pair.");

  ARGUMENTS
    + Argument ("input output", "list of all input and output tissue compartment files (see example usage).").type_various().allow_multiple();

  OPTIONS
    + Option ("mask", "the mask defines the data used to compute the intensity normalisation. This option is mandatory.").required ()
    + Argument ("image").type_image_in ()

    + Option ("order", "the maximum order of the polynomial basis used to fit the normalisation field in the log-domain. An order of 0 is equivalent to not allowing spatial variance of the intensity normalisation factor. (default: " + str(DEFAULT_POLY_ORDER) + ")")
    + Argument ("number").type_choice (poly_order_choices)

    + Option ("niter", "set the number of iterations. (default: " + str(DEFAULT_MAIN_ITER_VALUE) + ")")
    + Argument ("number").type_integer()

    + Option ("reference", "specify the (positive) reference value to which the summed tissue compartments will be normalised. "
                           "(default: " + str(DEFAULT_REFERENCE_VALUE, 6) + ", SH DC term for unit angular integral)")
    + Argument ("number").type_float (std::numeric_limits<default_type>::min())

    + Option ("balanced", "incorporate the per-tissue balancing factors into scaling of the output images "
                          "(NOTE: use of this option has critical consequences for AFD intensity normalisation; "
                          "should not be used unless these consequences are fully understood)")

    + OptionGroup ("Debugging options")

    + Option ("check_norm", "output the final estimated spatially varying intensity level that is used for normalisation.")
    + Argument ("image").type_image_out ()

    + Option ("check_mask", "output the final mask used to compute the normalisation. "
                            "This mask excludes regions identified as outliers by the optimisation process.")
    + Argument ("image").type_image_out ()

    + Option ("check_factors", "output the tissue balance factors computed during normalisation.")
    + Argument ("file").type_file_out ();

}

using ValueType = float;
using ImageType = Image<ValueType>;
using MaskType = Image<bool>;

// Function to get the number of basis vectors based on the desired order
int GetBasisVecs(int order)
{
  int n_basis_vecs;
    switch (order) {
      case 0:
        n_basis_vecs = 1;
        break;
      case 1:
        n_basis_vecs = 4;
        break;
      case 2:
        n_basis_vecs = 10;
        break;
      default:
        n_basis_vecs = 20;
        break;
      }
  return n_basis_vecs;
};

// Struct to get user specified number of basis functions
struct PolyBasisFunction { MEMALIGN (PolyBasisFunction)
  PolyBasisFunction(const int order) : n_basis_vecs (GetBasisVecs(order)) { };

  const int n_basis_vecs;

  FORCE_INLINE Eigen::MatrixXd operator () (const Eigen::Vector3& pos) {
    double x = pos[0];
    double y = pos[1];
    double z = pos[2];
    Eigen::MatrixXd basis(n_basis_vecs, 1);
    basis(0) = 1.0;
    if (n_basis_vecs < 4)
      return basis;

    basis(1) = x;
    basis(2) = y;
    basis(3) = z;
    if (n_basis_vecs < 10)
      return basis;

    basis(4) = x * x;
    basis(5) = y * y;
    basis(6) = z * z;
    basis(7) = x * y;
    basis(8) = x * z;
    basis(9) = y * z;
    if (n_basis_vecs < 20)
      return basis;

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

// Struct calculating the norm_field_log values
struct NormFieldLog {
   NormFieldLog (Eigen::MatrixXd norm_field_weights, Transform transform, struct PolyBasisFunction basis_function) : norm_field_weights (norm_field_weights), transform (transform), basis_function (basis_function){ }

   void operator () (ImageType& norm_field_log) {
       Eigen::Vector3 vox (norm_field_log.index(0), norm_field_log.index(1), norm_field_log.index(2));
       Eigen::Vector3 pos = transform.voxel2scanner * vox;
       norm_field_log.value() = basis_function (pos).col(0).dot (norm_field_weights.col(0));
   }

   Eigen::MatrixXd norm_field_weights;
   Transform transform;
   struct PolyBasisFunction basis_function;
};

// Struct calculating the initial summed_log values
struct SummedLog {
  SummedLog (const size_t n_tissue_types, Eigen::VectorXd balance_factors) : n_tissue_types (n_tissue_types), balance_factors (balance_factors) { }

  void operator () (ImageType& summed_log, ImageType& combined_tissue, ImageType& norm_field_image) {
      for (size_t j = 0; j < n_tissue_types; ++j) {
        combined_tissue.index(3) = j;
        summed_log.value() += balance_factors(j) * combined_tissue.value() / norm_field_image.value();
      }
  summed_log.value() = std::log(summed_log.value());
  }

  const size_t n_tissue_types;
  Eigen::VectorXd balance_factors;
};

// Function to perform outlier rejection
size_t OutlierRejection(float outlier_range, MaskType& mask, MaskType& initial_mask, Header header_3D, ImageType combined_tissue, ImageType norm_field_image, Eigen::VectorXd balance_factors, size_t num_voxels){

    auto summed_log = ImageType::scratch (header_3D, "Log of summed tissue volumes");
    ThreadedLoop (summed_log, 0, 3).run (SummedLog(combined_tissue.size(3), balance_factors), summed_log, combined_tissue, norm_field_image);
    threaded_copy (initial_mask, mask);

    vector<float> summed_log_values;
    summed_log_values.reserve (num_voxels);
    for (auto i = Loop (0, 3) (mask, summed_log); i; ++i) {
      if (mask.value())
        summed_log_values.push_back (summed_log.value());
    }

    num_voxels = summed_log_values.size();

    auto lower_quartile_it = summed_log_values.begin() + std::round ((float)num_voxels * 0.25f);
    std::nth_element (summed_log_values.begin(), lower_quartile_it, summed_log_values.end());
    float lower_quartile = *lower_quartile_it;
    auto upper_quartile_it = summed_log_values.begin() + std::round ((float)num_voxels * 0.75f);
    std::nth_element (lower_quartile_it, upper_quartile_it, summed_log_values.end());
    float upper_quartile = *upper_quartile_it;
    float lower_outlier_threshold = lower_quartile - outlier_range * (upper_quartile - lower_quartile);
    float upper_outlier_threshold = upper_quartile + outlier_range * (upper_quartile - lower_quartile);

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

return num_voxels;
};

// Function to define the output values at the beginning of the run () function
ImageType DefineOutput(vector<std::string> output_filenames, vector<Header> output_headers) {
  ImageType output_image;
  for (size_t j = 0; j < output_filenames.size(); ++j) {
     output_image = ImageType::scratch (output_headers[j], output_filenames[j]);
  }
return output_image;
};

// Function to compute the Choleski decompostion based on an N by n matrix and an N by 1 vector
Eigen::VectorXd Choleski(Eigen::MatrixXd X, Eigen::VectorXd y) {
   Eigen::MatrixXd M (X.cols(), X.cols());
   Eigen::VectorXd alpha (X.cols());
   M = X.transpose()*X;
   alpha = X.transpose()*y;
   Eigen::VectorXd res = M.llt().solve (alpha);
return res;
};

// Function to refine the mask
template<class InType>
void RefinedMask(InType input_images, MaskType& initial_mask, MaskType orig_mask, ProgressBar& input_progress, ImageType summed){
    for (size_t j = 0; j < input_images.size(); ++j) {
      input_progress++;
      ThreadedLoop(summed).run([](decltype(summed)& sum, decltype(input_images[0])& in){sum.value()+=in.value();}, summed, input_images[j]);
    }
ThreadedLoop(summed).run([](ImageType& summed, MaskType& initial_mask, MaskType& refined){refined.value() = ( std::isfinite(float(summed.value())) && summed.value() > 0.f && initial_mask.value() );}, summed, orig_mask, initial_mask);
};

// Function to solve for tissue balance factors
void BalFactSolver(Eigen::MatrixXd& X, Eigen::VectorXd& y, MaskType mask, ImageType combined_tissue, ImageType norm_field_image, size_t n_tissue_types){
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
};

// Function to solve for normalisation field weights in the log domain
void NormWeightsLog(Eigen::MatrixXd& norm_field_basis, Eigen::VectorXd& y, Eigen::VectorXd balance_factors, struct PolyBasisFunction basis_function, MaskType mask, ImageType& combined_tissue, Transform transform, size_t n_tissue_types, float log_norm_value){
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
};

// Function to compute log-norm scale parameter
void LogScale(double& lognorm_scale, uint32_t vox_count, MaskType mask, ImageType norm_field_log){
  if (vox_count) {
    struct LogNormScale {
      LogNormScale (double& lognorm_scale, uint32_t num_voxels) : lognorm_scale (lognorm_scale), num_voxels (num_voxels) { }
      FORCE_INLINE void operator () (decltype(mask) mask_in, decltype(norm_field_log) norm_field_lg) { if (mask_in.value ()){ lognorm_scale += norm_field_lg.value (); } lognorm_scale = std::exp(lognorm_scale / (double)num_voxels); }
      double& lognorm_scale;
      uint32_t num_voxels;
   };
   ThreadedLoop (mask).run (LogNormScale(lognorm_scale, vox_count), mask, norm_field_log);
  }
};

void run ()
{
    if (argument.size() % 2)
    throw Exception ("The number of arguments must be even, provided as pairs of each input and its corresponding output file.");

  const int order = get_option_value<int> ("order", DEFAULT_POLY_ORDER);
  PolyBasisFunction basis_function(order);

  vector<Adapter::Replicate<ImageType>> input_images;
  vector<Header> output_headers;
  vector<std::string> output_filenames;
  ImageType output_image;

  ProgressBar input_progress ("loading input images", 3*argument.size()/2);

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

  // Preparing default settings to the output images
  output_image = DefineOutput(output_filenames, output_headers);

  // Setting the n_tissue_types
  const size_t n_tissue_types = input_images.size();

  // Load the mask and refine the initial mask to exclude non-positive summed tissue components
  Header header_3D (input_images[0]);
  header_3D.ndim() = 3;
  header_3D.datatype() = DataType::Float32;
  auto opt = get_options ("mask");

  auto orig_mask = MaskType::open (opt[0][0]);
  Header mask_header (orig_mask);
  mask_header.ndim() = 3;
  mask_header.datatype() = DataType::Bit;
  Stride::set (mask_header, header_3D);

  auto initial_mask = MaskType::scratch (mask_header, "Initial processing mask");
  auto mask = MaskType::scratch (mask_header, "Processing mask");
  auto prev_mask = MaskType::scratch (mask_header, "Previous processing mask");

  {
    auto summed = ImageType::scratch (header_3D, "Summed tissue volumes");
    RefinedMask(input_images, initial_mask, orig_mask, input_progress, summed);
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
     ThreadedLoop (combined_tissue, 0, 3).run ([](decltype(combined_tissue)& comb, decltype(input_images[0]) in) { comb.value() = std::max<float>(in.value (), 0.f); },combined_tissue, input_images[i]);
  }

  size_t num_voxels = 0;
  ThreadedLoop (mask, 0, 3).run ([&num_voxels](decltype(mask) mask) { if (mask.value()) ++num_voxels; }, mask);

  if (!num_voxels)
    throw Exception ("Mask contains no valid voxels.");


  const float reference_value = get_option_value ("reference", DEFAULT_REFERENCE_VALUE);
  const float log_ref_value = std::log (reference_value);
  const size_t max_iter = get_option_value ("niter", DEFAULT_MAIN_ITER_VALUE);
  const size_t max_balance_iter = DEFAULT_BALANCE_MAXITER_VALUE;

  // Initialise normalisation fields in both image and log domain
  Eigen::MatrixXd norm_field_weights;

  auto norm_field_image = ImageType::scratch (header_3D, "Normalisation field (intensity)");
  auto norm_field_log = ImageType::scratch (header_3D, "Normalisation field (log-domain)");
  ThreadedLoop (norm_field_image).run ([](decltype(norm_field_image)& in) { in.value() = 1.0; },norm_field_image);

  Eigen::VectorXd balance_factors (Eigen::VectorXd::Ones (n_tissue_types));

  size_t iter = 1;

  input_progress.done ();
  ProgressBar progress ("performing log-domain intensity normalisation", max_iter);

  // Pre-writing the summed_log variable and the vox_count and new_vox_count variables
  size_t vox_count, new_vox_count;

  // Perform an initial outlier rejection prior to the first iteration
  vox_count = OutlierRejection(3.f, mask, initial_mask, header_3D, combined_tissue, norm_field_image, balance_factors, num_voxels);

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
        Eigen::MatrixXd X (vox_count, n_tissue_types);
        Eigen::VectorXd y (Eigen::VectorXd::Ones (vox_count));
        BalFactSolver(X, y, mask, combined_tissue, norm_field_image, n_tissue_types);
        balance_factors = Choleski(X, y);

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
      new_vox_count = OutlierRejection(1.5f, mask, initial_mask, header_3D, combined_tissue, norm_field_image, balance_factors, vox_count);

      // Check for convergence
      balance_converged = true;

      if (new_vox_count != vox_count){
         balance_converged = false;
         vox_count = new_vox_count;
      }else{
         for (auto i = Loop (0, 3) (mask, prev_mask); i; ++i) {
            if (mask.value() != prev_mask.value()) {
            balance_converged = false;
            vox_count = new_vox_count;
            break;
            }
         }
      }

      threaded_copy (mask, prev_mask);

      balance_iter++;
    }


    // Solve for normalisation field weights in the log domain
    Transform transform (mask);
    Eigen::MatrixXd norm_field_basis (vox_count, basis_function.n_basis_vecs);
    Eigen::VectorXd y (vox_count);
    NormWeightsLog(norm_field_basis, y, balance_factors, basis_function, mask, combined_tissue, transform, n_tissue_types, log_ref_value);
    norm_field_weights = Choleski(norm_field_basis, y);

    // Generate normalisation field in the log domain
    ThreadedLoop (norm_field_log, 0, 3).run (NormFieldLog(norm_field_weights, transform, basis_function), norm_field_log);

    // Generate normalisation field in the image domain
    ThreadedLoop (norm_field_image).run([](decltype(norm_field_image)& out, decltype(norm_field_log) in){out.value() = std::exp (in.value());}, norm_field_image, norm_field_log);

    progress++;
    iter++;
  }

  progress.done();

  ProgressBar output_progress("writing output images", output_filenames.size());

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

  opt = get_options ("check_factors");
  if (opt.size()) {
    File::OFStream factors_output (opt[0][0]);
    factors_output << balance_factors;
  }

  // Compute log-norm scale parameter (geometric mean of normalisation field in outlier-free mask).
  double lognorm_scale (0.0);
  LogScale(lognorm_scale, vox_count, mask, norm_field_log);
  const bool output_balanced = get_options("balanced").size();

  for (size_t j = 0; j < output_filenames.size(); ++j) {
    output_progress++;

    float balance_multiplier = 1.0f;
    output_headers[j].keyval()["lognorm_scale"] = str(lognorm_scale);
    if (output_balanced) {
      balance_multiplier = balance_factors[j];
      output_headers[j].keyval()["lognorm_balance"] = str(balance_multiplier);
    }
    output_image = ImageType::create (output_filenames[j], output_headers[j]);
    const size_t n_vols = input_images[j].size(3);
    const Eigen::VectorXf zero_vec = Eigen::VectorXf::Zero (n_vols);

  struct ReadInOutput {
     ReadInOutput (Eigen::VectorXf zero_vec, float balance_multiplier) : zero_vec (zero_vec), balance_multiplier (balance_multiplier) { }
     FORCE_INLINE void operator () (decltype(output_image)& out_im, decltype(input_images[0]) in_im, decltype(norm_field_image) norm_field_im) { in_im.index(3) = 0; if (in_im.value() < 0.f) { out_im.row(3) = zero_vec; } else { out_im.row(3) = Eigen::VectorXf{in_im.row(3)} * balance_multiplier / norm_field_im.value(); } }
     Eigen::VectorXf zero_vec;
     float balance_multiplier;
  };
  ThreadedLoop (output_image, 0, 3).run (ReadInOutput(zero_vec, balance_multiplier), output_image, input_images[j], norm_field_image);
  }
}
