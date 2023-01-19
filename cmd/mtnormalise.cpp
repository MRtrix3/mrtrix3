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
  AUTHOR = "Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au), "
    "David Raffelt (david.raffelt@florey.edu.au), Jonas Rosnarho-Tornstrand (jonas.rosnarho-tornstrand@kcl.ac.uk) "
    "and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Multi-tissue informed log-domain intensity normalisation";

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

    + Option ("order", "the maximum order of the polynomial basis used to fit the normalisation field in the log-domain. "
        "An order of 0 is equivalent to not allowing spatial variance of the intensity normalisation factor. "
        "(default: " + str(DEFAULT_POLY_ORDER) + ")")
    + Argument ("number").type_choice (poly_order_choices)

    + Option ("niter", "set the number of iterations. The first (and potentially only) entry applies to the main loop. "
        "If supplied as a comma-separated list of integers, the second entry applies to the inner loop to update the balance factors "
        "(default: " + str(DEFAULT_MAIN_ITER_VALUE) + "," + str(DEFAULT_BALANCE_MAXITER_VALUE) + ").")
    + Argument ("number").type_sequence_int()

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

    REFERENCES
    + "Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. " // Internal
    "Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. "
    "In Proc. ISMRM, 2017, 26, 3541"
    + "Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. " // Internal
    "Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. "
    "In Proc. ISMRM, 2021, 29, 2472";
;

}

using ValueType = float;
using ImageType = Image<ValueType>;
using MaskType = Image<bool>;
using IndexType = Image<uint32_t>;

// Function to get the number of basis vectors based on the desired order
int num_basis_vec_for_order  (int order)
{
  switch (order) {
    case 0: return 1;
    case 1: return 4;
    case 2: return 10;
    default: return 20;
  }
  assert (false);
  return -1;
};





// Struct to get user specified number of basis functions
struct PolyBasisFunction { MEMALIGN (PolyBasisFunction)
  PolyBasisFunction(const int order) :
    n_basis_vecs (num_basis_vec_for_order (order)) { };

  const int n_basis_vecs;

  FORCE_INLINE Eigen::VectorXd operator () (const Eigen::Vector3d& pos) const {
    double x = pos[0];
    double y = pos[1];
    double z = pos[2];
    Eigen::VectorXd basis (n_basis_vecs);
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








IndexType index_mask_voxels (size_t& num_voxels)
{
  auto opt = get_options ("mask");
  auto mask = MaskType::open (opt[0][0]);
  check_effective_dimensionality (mask, 3);

  if (voxel_count (mask, 0, 3) >= std::numeric_limits<uint32_t>::max()-1)
    throw Exception ("mask size exceeds maximum supported using 32-bit integer");
  num_voxels = 0;

  Header header (mask);
  header.ndim() = 3;
  header.datatype() = DataType::UInt32;
  IndexType index = IndexType::scratch (header, "index");

  for (auto l = Loop(0, 3) (mask, index); l; ++l) {
    if (mask.value())
      index.value() = num_voxels++;
    else
      index.value() = std::numeric_limits<uint32_t>::max();
  }

  if (!num_voxels)
    throw Exception ("Mask contains no valid voxels.");

  INFO ("mask image contains " + str(num_voxels) + " voxels");

  return index;
}




Eigen::MatrixXd initialise_basis (IndexType& index, size_t num_voxels, int order)
{
  struct BasisInitialiser { NOMEMALIGN
    BasisInitialiser (const Transform& transform, const PolyBasisFunction& basis_function, Eigen::MatrixXd& basis) :
      basis_function (basis_function),
      transform (transform),
      basis (basis) { }

    void operator() (IndexType& index) {
      const uint32_t idx = index.value();
      if (idx != std::numeric_limits<uint32_t>::max()) {
        assert (idx < basis.rows());
        Eigen::Vector3d vox (index.index(0), index.index(1), index.index(2));
        Eigen::Vector3d pos = transform.voxel2scanner * vox;
        basis.row(idx) = basis_function (pos);
      }
    }

    const PolyBasisFunction& basis_function;
    const Transform& transform;
    Eigen::MatrixXd& basis;
  };

  INFO ("initialising basis...");

  PolyBasisFunction basis_function (order);
  Transform transform (index);
  Eigen::MatrixXd basis (num_voxels, num_basis_vec_for_order (order));

  ThreadedLoop (index, 0, 3, 2).run (BasisInitialiser (transform, basis_function, basis), index);
  return basis;
}






void load_data (Eigen::MatrixXd& data, const std::string& image_name, IndexType& index)
{
  static int num = 0;

  auto in = ImageType::open (image_name);
  check_dimensions (index, in, 0, 3);

  struct Loader { NOMEMALIGN
    public:
      Loader (Eigen::MatrixXd& data, int num) : data (data), num (num) { }

      void operator() (ImageType& in, IndexType& index) {
        const uint32_t idx = index.value();
        if (idx != std::numeric_limits<uint32_t>::max())
          data(idx, num) = std::max<ValueType> (in.value(), 0.0);
      }
      Eigen::MatrixXd& data;
      const int num;
  };

  ThreadedLoop (in, 0, 3, 2).run (Loader (data, num), in, index);
  ++num;
}



inline bool lessthan_NaN (const double& a, const double& b) {
  if (std::isnan (a))
    return true;
  if (std::isnan (b))
    return false;
  return a<b;
}



size_t detect_outliers (
    double outlier_range,
    const Eigen::MatrixXd& data,
    const Eigen::VectorXd& field,
    const Eigen::VectorXd& balance_factors,
    Eigen::VectorXd& weights)
{
  Eigen::VectorXd summed_log = (data * balance_factors).cwiseQuotient (field).array().log();
  Eigen::VectorXd summed_log_sorted = summed_log;
  const size_t lower_quartile_idx = std::round (field.size() * 0.25);
  const size_t upper_quartile_idx = std::round (field.size() * 0.75);

  std::nth_element (summed_log_sorted.data(), summed_log_sorted.data() + lower_quartile_idx,
      summed_log_sorted.data() + summed_log_sorted.size(), lessthan_NaN);
  double lower_quartile = summed_log_sorted[lower_quartile_idx];

  std::nth_element (summed_log_sorted.data(), summed_log_sorted.data() + upper_quartile_idx,
      summed_log_sorted.data() + summed_log_sorted.size(), lessthan_NaN);
  double upper_quartile = summed_log_sorted[upper_quartile_idx];

  INFO ("  outlier rejection quartiles: [ " + str(lower_quartile) + " " + str(upper_quartile) + " ]");

  double lower_outlier_threshold = lower_quartile - outlier_range * (upper_quartile - lower_quartile);
  double upper_outlier_threshold = upper_quartile + outlier_range * (upper_quartile - lower_quartile);

  struct SetWeight { NOMEMALIGN
    size_t& changed;
    const double lower_outlier_threshold, upper_outlier_threshold;

    double operator() (double v, double w) const {
      v = std::isfinite (v) && v >= lower_outlier_threshold && v <= upper_outlier_threshold;
      if (v != w)
        ++changed;
      return v;
    }
  };

  size_t changed = 0;
  SetWeight set_weight = { changed, lower_outlier_threshold, upper_outlier_threshold };
  weights = summed_log.binaryExpr (weights, set_weight);

  return changed;
}







void compute_balance_factors (
    const Eigen::MatrixXd& data,
    const Eigen::VectorXd& field,
    const Eigen::VectorXd& weights,
    Eigen::VectorXd& balance_factors)
{
  Eigen::MatrixXd scaled_data = data.transpose().array().rowwise() / field.transpose().array();
  for (ssize_t n = 0; n < scaled_data.cols(); ++n) {
    if (!weights[n])
      scaled_data.col(n).array() = 0.0;
  }
  Eigen::MatrixXd HtH (data.cols(), data.cols());
  HtH.triangularView<Eigen::Lower>() = scaled_data * scaled_data.transpose();
  Eigen::LLT<Eigen::MatrixXd> llt;
  balance_factors.noalias() = llt.compute (HtH.triangularView<Eigen::Lower>()).solve (scaled_data * Eigen::VectorXd::Ones(field.size()));

  // Ensure our balance factors satisfy the condition that sum(log(balance_factors)) = 0
  if (!balance_factors.allFinite() || (balance_factors.array() <= 0.0).any())
    throw Exception ("Non-positive tissue balance factor was computed."
        " Balance factors: " + str(balance_factors.transpose()));

  balance_factors /= std::exp (balance_factors.array().log().sum() / data.cols());
}




void update_field (
    const double log_norm_value,
    const Eigen::MatrixXd& basis,
    const Eigen::MatrixXd& data,
    const Eigen::VectorXd& balance_factors,
    const Eigen::VectorXd& weights,
    Eigen::VectorXd& field_coeffs,
    Eigen::VectorXd& field)
{
  struct LogWeight { NOMEMALIGN
    double operator() (double sum, double weight) const {
      return sum > 0.0 ? weight * (std::log(sum) - log_norm_value) : 0.0;
    }
    const double log_norm_value;
  };
  LogWeight logweight = { log_norm_value };

  Eigen::VectorXd logsum = data * balance_factors;
  logsum = logsum.binaryExpr (weights, logweight);

  Eigen::MatrixXd HtH = Eigen::MatrixXd::Zero (basis.cols(), basis.cols());
  HtH.selfadjointView<Eigen::Lower>().rankUpdate ((basis.transpose().array().rowwise() * weights.transpose().array()).matrix());

  Eigen::LLT<Eigen::MatrixXd> llt;
  field_coeffs.noalias() = llt.compute (HtH.selfadjointView<Eigen::Lower>()).solve (basis.transpose() * logsum);

  field.noalias() = (basis * field_coeffs).array().exp().matrix();
}





ImageType compute_full_field (int order, const Eigen::VectorXd& field_coeffs, const IndexType& index)
{
  Header header (index);
  header.datatype() = DataType::Float32;

  auto out = ImageType::scratch (header, "full field");
  Transform transform (out);

  struct FieldWriter { NOMEMALIGN
    void operator() (ImageType& field) const {
      Eigen::Vector3d vox (field.index(0), field.index(1), field.index(2));
      Eigen::Vector3d pos = transform.voxel2scanner * vox;
      field.value() = std::exp (basis_function (pos).dot (field_coeffs));
    }

    const PolyBasisFunction& basis_function;
    const Eigen::VectorXd& field_coeffs;
    const Transform& transform;
  };

  PolyBasisFunction basis_function (order);

  FieldWriter writer = { basis_function, field_coeffs, transform };

  ThreadedLoop (out, 0, 3).run (writer, out);

  return out;
}





void write_weights (const Eigen::VectorXd& data, IndexType& index, const std::string& output_file_name)
{
  Header header (index);
  header.datatype() = DataType::Float32;

  auto out = ImageType::create (output_file_name, header);

  struct Write { NOMEMALIGN
    void operator() (ImageType& out, IndexType& index) const {
      const uint32_t idx = index.value();
      if (idx != std::numeric_limits<uint32_t>::max()) {
        out.value() = data[idx];
      }
    }
    const Eigen::VectorXd& data;
  } write = { data };

  ThreadedLoop (index, 0, 3).run (write, out, index);
}



void write_output (
    const std::string& original,
    const std::string& corrected,
    bool output_balanced,
    double balance_factor,
    ImageType& field,
    double lognorm_scale)
{
  using ReplicatorType = Adapter::Replicate<ImageType>;

  struct Scaler { NOMEMALIGN
    void operator() (ImageType& original, ImageType& corrected, ReplicatorType& field) const {
      corrected.value() = balance_factor * original.value() / field.value();
    }
    const double balance_factor;
  };

  auto in = ImageType::open (original);
  Header header (in);
  header.datatype() = DataType::Float32;
  header.keyval()["lognorm_scale"] = str(lognorm_scale);
  if (output_balanced)
    header.keyval()["lognorm_balance"] = str(balance_factor);
  else
    balance_factor = 1.0;
  auto out = ImageType::create (corrected, header);

  Header header_broadcast (field);
  header_broadcast.ndim() = 4;
  header_broadcast.size(3) = in.ndim() > 3 ? in.size(3) : 1;
  ReplicatorType field_broadcast (field, header_broadcast);

  Scaler scaler = { balance_factor };
  ThreadedLoop (in).run (scaler, in, out, field_broadcast);
}





void run ()
{
  if (argument.size() % 2)
    throw Exception ("The number of arguments must be even, provided as pairs of each input and its corresponding output file.");
  if (argument.size() == 2)
    WARN("Only one contrast provided. If multi-tissue CSD was performed, provide all components to mtnormalise.");

  const int order = get_option_value<int> ("order", DEFAULT_POLY_ORDER);
  const float reference_value = get_option_value ("reference", DEFAULT_REFERENCE_VALUE);
  const float log_ref_value = std::log (reference_value);

  size_t max_iter = DEFAULT_MAIN_ITER_VALUE;
  size_t max_balance_iter = DEFAULT_BALANCE_MAXITER_VALUE;
  auto opt = get_options ("niter");
  if (opt.size()) {
    vector<size_t> num = parse_ints<size_t> (opt[0][0]);
    if (num.size() < 1 && num.size() > 2)
      throw Exception ("unexpected number of entries provided to option \"-niter\"");
    for (auto n : num)
      if (!n)
        throw Exception ("number of iterations must be nonzero");

    max_iter = num[0];
    if (num.size() > 1)
      max_balance_iter = num[1];
  }

  // Setting the n_tissue_types
  const size_t n_tissue_types = argument.size()/2;

  size_t num_voxels;
  auto index = index_mask_voxels (num_voxels);

  Eigen::MatrixXd data (num_voxels, n_tissue_types);
  for (size_t n = 0; n < n_tissue_types; ++n) {
    if (Path::exists (argument[2*n+1]) && !App::overwrite_files)
      throw Exception ("Output file \"" + argument[2*n+1] + "\" already exists. (use -force option to force overwrite)");
    load_data (data, argument[2*n], index);
  }

  size_t num_non_finite = (!data.array().isFinite()).count();
  if (num_non_finite > 0) {
    WARN ("Input data contain " + str(num_non_finite) + " non-finite voxel" + ( num_non_finite > 1 ? "s" : "" ));
    WARN ("  Results may be affected if the data contain many non-finite values");
    WARN ("  Please refine your mask to avoid non-finite values if this is a problem");
  }

  auto basis = initialise_basis (index, num_voxels, order);

  struct finite_and_positive { NOMEMALIGN double operator() (double v) const { return std::isfinite(v) && v > 0.0; } };
  Eigen::VectorXd weights = data.rowwise().sum().unaryExpr (finite_and_positive());

  Eigen::VectorXd field = Eigen::VectorXd::Ones (num_voxels);
  Eigen::VectorXd field_coeffs (basis.cols());
  Eigen::VectorXd balance_factors (Eigen::VectorXd::Ones (n_tissue_types));


  {
    size_t iter = 0;
    ProgressBar progress ("performing log-domain intensity normalisation", max_iter);

    size_t outliers_changed = detect_outliers (3.0, data, field, balance_factors, weights);

    while (++iter <= max_iter) {
      INFO ("Iteration: " + str(iter));

      size_t balance_iter = 1;

      // Iteratively compute tissue balance factors with outlier rejection
      do {

        DEBUG ("Balance and outlier rejection iteration " + str(balance_iter) + " starts.");

        if (n_tissue_types > 1) {
          compute_balance_factors (data, field, weights, balance_factors);
          INFO ("  balance factors (" + str(balance_iter) + "): " + str(balance_factors.transpose()));
        }

        outliers_changed = detect_outliers (1.5, data, field, balance_factors, weights);

      } while (outliers_changed && balance_iter++ < max_balance_iter);


      update_field (log_ref_value, basis, data, balance_factors, weights, field_coeffs, field);

      progress++;
    }

  }



  auto full_field = compute_full_field (order, field_coeffs, index);

  opt = get_options ("check_norm");
  if (opt.size()) {
    auto out = ImageType::create (opt[0][0], full_field);
    threaded_copy (full_field, out);
  }

  opt = get_options ("check_mask");
  if (opt.size())
    write_weights (weights, index, opt[0][0]);

  opt = get_options ("check_factors");
  if (opt.size()) {
    File::OFStream factors_output (opt[0][0]);
    factors_output << balance_factors.transpose() << "\n";
  }

  double lognorm_scale = std::exp ((field.array().log() * weights.array()).sum() / weights.sum());

  const bool output_balanced = get_options("balanced").size();
  for (size_t n = 0; n < n_tissue_types; ++n)
    write_output (argument[2*n], argument[2*n+1], output_balanced, balance_factors[n], full_field, lognorm_scale);
}

