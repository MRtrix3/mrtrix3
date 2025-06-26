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

#include "denoise/recon.h"

#include "denoise/denoise.h"
#include "math/math.h"

namespace MR::Denoise {

template <typename F>
Recon<F>::Recon(const Image<F> &image,
                std::shared_ptr<Subsample> subsample,
                std::shared_ptr<Kernel::Base> kernel,
                std::shared_ptr<Estimator::Base> estimator,
                filter_type filter,
                aggregator_type aggregator,
                Exports &exports,
                const ssize_t preconditioner_rank)
    : Estimate<F>(image, subsample, kernel, estimator, exports, preconditioner_rank, true),
      filter(filter),
      aggregator(aggregator),
      // FWHM = 2 x cube root of spacings between kernels
      gaussian_multiplier(-std::log(2.0) /                                                          //
                          Math::pow2(std::cbrt(subsample->get_factors()[0] * image.spacing(0)       //
                                               * subsample->get_factors()[1] * image.spacing(1)     //
                                               * subsample->get_factors()[2] * image.spacing(2)))), //
      w(std::min(Estimate<F>::m, kernel->estimated_size())),
      Xr(Estimate<F>::m, aggregator == aggregator_type::EXCLUSIVE ? 1 : kernel->estimated_size()) {}

template <typename F> void Recon<F>::operator()(Image<F> &dwi, Image<F> &out) {

  if (!Estimate<F>::subsample->process({dwi.index(0), dwi.index(1), dwi.index(2)}))
    return;

  Estimate<F>::operator()(dwi);

  const ssize_t n = Estimate<F>::patch.voxels.size();
  const ssize_t r = std::min(Estimate<F>::m, n);
  const ssize_t rz = rank_zero(Estimate<F>::m, n, Estimate<F>::preconditioner_rank);
  const ssize_t rnz = rank_nonzero(Estimate<F>::m, n, Estimate<F>::preconditioner_rank);
  const ssize_t qnz = dimlong_nonzero(Estimate<F>::m, n, Estimate<F>::preconditioner_rank);
  const double beta = double(rnz) / double(qnz);

  if (r > w.size())
    w.resize(r);
  if (aggregator != aggregator_type::EXCLUSIVE && n > Xr.cols())
    Xr.resize(Estimate<F>::m, n);
#ifndef NDEBUG
  w.fill(std::numeric_limits<default_type>::signaling_NaN());
  Xr.fill(std::numeric_limits<default_type>::signaling_NaN());
#endif

  // Generate weights vector
  double sum_weights = 0.0;
  double sum_variance = 0.0;
  ssize_t out_rank = 0;
  if (bool(Estimate<F>::threshold)) {
    switch (filter) {
    case filter_type::OPTSHRINK: {
      w.head(rz).setZero();
      const double transition = 1.0 + std::sqrt(beta);
      for (ssize_t i = rz; i != r; ++i) {
        // TODO For non-binary determination of weights for optimal shrinkage,
        //   should the expression be identical between BDCSVD and SelfAdjointEigenSolver?
        //   Or eg. is one equivalent to scaling singular values whereas the other is equivalent to scaling eigenvalues?
#ifdef DWIDENOISE2_USE_BDCSVD
        const double lam = Estimate<F>::s[i] / qnz;
#else
        const double lam = std::max(Estimate<F>::s[i], 0.0) / qnz;
#endif
        const double y = std::sqrt(lam / Estimate<F>::threshold.sigma2);
        // const double y = lam / Estimate<F>::threshold.sigma2;
        double nu = 0.0;
        if (y > transition) {
          // Occasionally floating-point precision will drive this calculation to fractionally greater than y,
          //   which will erroneously yield a weight fractionally greater than 1.0
          nu = std::min(y, std::sqrt(Math::pow2(Math::pow2(y) - beta - 1.0) - (4.0 * beta)) / y);
          ++out_rank;
        }
        w[i] = lam > 0.0 ? (nu / y) : 0.0;
        assert(w[i] >= 0.0 && w[i] <= 1.0);
        sum_weights += w[i];
#ifdef DWIDENOISE2_USE_BDCSVD
        sum_variance += w[i] * Estimate<F>::s[i];
#else
        sum_variance += w[i] * std::max(Estimate<F>::s[i], 0.0);
#endif
      }
    } break;
    case filter_type::OPTTHRESH: {
      const std::map<double, double>::const_iterator it = beta2lambdastar.find(beta);
      double lambda_star = 0.0;
      if (it == beta2lambdastar.end()) {
        lambda_star =
            sqrt(2.0 * (beta + 1.0) + ((8.0 * beta) / (beta + 1.0 + std::sqrt(Math::pow2(beta) + 14.0 * beta + 1.0))));
        beta2lambdastar[beta] = lambda_star;
      } else {
        lambda_star = it->second;
      }
      const double tau_star = lambda_star * std::sqrt(qnz) * std::sqrt(Estimate<F>::threshold.sigma2);
      // TODO Unexpected requisite square applied to qnz here
      const double threshold = tau_star * Math::pow2(qnz);
      w.head(rz).setZero();
      for (ssize_t i = rz; i != r; ++i) {
        if (Estimate<F>::s[i] >= threshold) {
          w[i] = 1.0;
          ++out_rank;
#ifdef DWIDENOISE2_USE_BDCSVD
          sum_variance += Estimate<F>::s[i];
#else
          sum_variance += std::max(Estimate<F>::s[i], 0.0);
#endif
        } else {
          w[i] = 0.0;
        }
      }
      sum_weights = out_rank;
    } break;
    case filter_type::TRUNCATE:
      out_rank = r - Estimate<F>::threshold.cutoff_p;
      w.head(Estimate<F>::threshold.cutoff_p).setZero();
      w.segment(Estimate<F>::threshold.cutoff_p, out_rank).setOnes();
      sum_weights = double(out_rank);
      sum_variance += w.head(r).matrix().dot(Estimate<F>::s.head(r).matrix());
      break;
    default:
      assert(false);
    }
    assert(std::isfinite(sum_weights));
  } else { // Threshold for this patch is invalid
    // Erring on the conservative side:
    //   If the decomposition fails, or a threshold can't be found,
    //   copy the input data to the output data as-is,
    //   regardless of whether performing overcomplete local PCA
    w.head(r).setOnes();
    out_rank = r;
    sum_weights = r;
#ifdef DWIDENOISE2_USE_BDCSVD
    sum_variance = Estimate<F>::s.sum();
#else
    sum_variance = Estimate<F>::s.unaryExpr([](double i) { return std::max(i, 0.0); }).sum();
#endif
  }
  assert(w.head(r).allFinite());
#ifdef DWIDENOISE2_USE_BDCSVD
  const double variance_removed = 1.0 - sum_variance / Estimate<F>::s.sum();
#else
  const double variance_removed =                                                                     //
      1.0 - sum_variance / Estimate<F>::s.unaryExpr([](double f) { return std::max(0.0, f); }).sum(); //
#endif

  // Recombine data using only eigenvectors above threshold
  // If only the data computed when this voxel was the centre of the patch
  //   is to be used for synthesis of the output image,
  //   then only that individual column needs to be reconstructed;
  //   if however the result from this patch is to contribute to the synthesized image
  //   for all voxels that were utilised within this patch,
  //   then we need to instead compute the full projection
#ifdef DWIDENOISE2_USE_BDCSVD
  const Eigen::Matrix<F, Eigen::Dynamic, 1> wrev = w.head(r).reverse().cast<F>();
#endif
  switch (aggregator) {
  case aggregator_type::EXCLUSIVE: {
    assert(Estimate<F>::patch.centre_index >= 0);
    if (bool(Estimate<F>::threshold)) {
#ifdef DWIDENOISE2_USE_BDCSVD
      assert(Estimate<F>::SVD.matrixU().allFinite());
      assert(Estimate<F>::SVD.matrixV().allFinite());
      assert(wrev.allFinite());
      assert(Estimate<F>::SVD.singularValues().allFinite());
      // TODO Re-try reconstruction without use of V:
      //   https://github.com/MRtrix3/mrtrix3/pull/2906/commits/eb34f3c57dd460d2b3bd86b9653066be15e916c6
      // It might be that in the case of anything other than EXCLUSIVE,
      //   computing V is no more expensive than doing the full patch reconstruction in its absence,
      //   whereas for EXCLUSIVE since only a small portion of V is used it's worthwhile
      Xr.noalias() = Estimate<F>::SVD.matrixU() *                                                       //
                     (wrev.array() * Estimate<F>::SVD.singularValues().array()).matrix().asDiagonal() * //
                     Estimate<F>::SVD.matrixV().row(Estimate<F>::patch.centre_index).adjoint();         //
#else
      if (Estimate<F>::m <= n)
        Xr.noalias() =                                               //
            Estimate<F>::eig.eigenvectors() *                        //
            (w.head(r).cast<F>().matrix().asDiagonal() *             //
             (Estimate<F>::eig.eigenvectors().adjoint() *            //
              Estimate<F>::X.col(Estimate<F>::patch.centre_index))); //
      else
        Xr.noalias() =                                                                          //
            Estimate<F>::X.leftCols(n) *                                                        //
            (Estimate<F>::eig.eigenvectors() *                                                  //
             (w.head(r).cast<F>().matrix().asDiagonal() *                                       //
              Estimate<F>::eig.eigenvectors().adjoint().col(Estimate<F>::patch.centre_index))); //
#endif
      assert(Xr.allFinite());
    } else {
      // In the case of -aggregator exclusive,
      //   where a decomposition fails or we can't find a threshold,
      //   we simply copy the input data into the output image
      Xr.noalias() = Estimate<F>::X.col(Estimate<F>::patch.centre_index);
    }
    assign_pos_of(dwi).to(out);
    out.row(3) = Xr.col(0);
    if (Estimate<F>::exports.sum_aggregation.valid()) {
      assign_pos_of(dwi, 0, 3).to(Estimate<F>::exports.sum_aggregation);
      Estimate<F>::exports.sum_aggregation.value() = 1.0;
    }
    if (Estimate<F>::exports.rank_output.valid()) {
      assign_pos_of(dwi, 0, 3).to(Estimate<F>::exports.rank_output);
      Estimate<F>::exports.rank_output.value() = out_rank;
    }
  } break;
  default: { // All aggregators other than EXCLUSIVE
    if (!Estimate<F>::threshold) {
      Xr.leftCols(n).noalias() = Estimate<F>::X.leftCols(n);
#ifdef DWIDENOISE2_USE_BDCSVD
    } else {
      Xr.leftCols(n).noalias() = Estimate<F>::SVD.matrixU() *                                                       //
                                 (wrev.array() * Estimate<F>::SVD.singularValues().array()).matrix().asDiagonal() * //
                                 Estimate<F>::SVD.matrixV().adjoint();                                              //
#else
    } else if (Estimate<F>::m <= n) {
      Xr.leftCols(n).noalias() =                        //
          Estimate<F>::eig.eigenvectors() *             //
          (w.head(r).cast<F>().matrix().asDiagonal() *  //
           (Estimate<F>::eig.eigenvectors().adjoint() * //
            Estimate<F>::X.leftCols(n)));               //
    } else {
      Xr.leftCols(n).noalias() =                         //
          Estimate<F>::X.leftCols(n) *                   //
          (Estimate<F>::eig.eigenvectors() *             //
           (w.head(r).cast<F>().matrix().asDiagonal() *  //
            Estimate<F>::eig.eigenvectors().adjoint())); //
#endif
    }
    assert(Xr.leftCols(n).allFinite());
    // Undo prior within-patch variance-stabilising transform
    if (std::isfinite(Estimate<F>::patch.centre_noise)) {
      for (ssize_t i = 0; i != n; ++i)
        if (Estimate<F>::patch.voxels[i].noise_level > 0.0)
          Xr.col(i) *= Estimate<F>::patch.voxels[i].noise_level / Estimate<F>::patch.centre_noise;
    }
    std::lock_guard<std::mutex> lock(Estimate<F>::mutex);
    for (size_t voxel_index = 0; voxel_index != Estimate<F>::patch.voxels.size(); ++voxel_index) {
      assign_pos_of(Estimate<F>::patch.voxels[voxel_index].index, 0, 3).to(out);
      assign_pos_of(Estimate<F>::patch.voxels[voxel_index].index).to(Estimate<F>::exports.sum_aggregation);
      double weight = std::numeric_limits<double>::signaling_NaN();
      switch (aggregator) {
      case aggregator_type::EXCLUSIVE:
        assert(false);
        break;
      case aggregator_type::GAUSSIAN:
        weight = std::exp(gaussian_multiplier * Estimate<F>::patch.voxels[voxel_index].sq_distance);
        break;
      case aggregator_type::INVL0:
        weight = 1.0 / (1 + out_rank);
        break;
      case aggregator_type::RANK:
        weight = out_rank;
        break;
      case aggregator_type::UNIFORM:
        weight = 1.0;
        break;
      }
      out.row(3) += weight * Xr.col(voxel_index);
      Estimate<F>::exports.sum_aggregation.value() += weight;
      if (Estimate<F>::exports.rank_output.valid()) {
        assign_pos_of(Estimate<F>::patch.voxels[voxel_index].index, 0, 3).to(Estimate<F>::exports.rank_output);
        Estimate<F>::exports.rank_output.value() += weight * out_rank;
      }
    }
  } break;
  }

  auto ss_index = Estimate<F>::subsample->in2ss({dwi.index(0), dwi.index(1), dwi.index(2)});
  if (Estimate<F>::exports.sum_optshrink.valid()) {
    assign_pos_of(ss_index, 0, 3).to(Estimate<F>::exports.sum_optshrink);
    Estimate<F>::exports.sum_optshrink.value() = sum_weights;
  }
  if (Estimate<F>::exports.variance_removed.valid()) {
    assign_pos_of(ss_index, 0, 3).to(Estimate<F>::exports.variance_removed);
    Estimate<F>::exports.variance_removed.value() = variance_removed;
  }
}

template class Recon<float>;
template class Recon<cfloat>;
template class Recon<double>;
template class Recon<cdouble>;

} // namespace MR::Denoise
