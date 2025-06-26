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

#include "denoise/estimate.h"

#include <limits>

#include "interp/cubic.h"
#include "math/math.h"

namespace MR::Denoise {

template <typename F>
Estimate<F>::Estimate(const Image<F> &image,
                      std::shared_ptr<Subsample> subsample,
                      std::shared_ptr<Kernel::Base> kernel,
                      std::shared_ptr<Estimator::Base> estimator,
                      Exports &exports,
                      const ssize_t preconditioner_rank,
                      const bool enable_recon)
    : m(image.size(3)),
      subsample(subsample),
      kernel(kernel),
      estimator(estimator),
      preconditioner_rank(preconditioner_rank),
      enable_recon(enable_recon),
      X(m, kernel->estimated_size()),
#ifdef DWIDENOISE2_USE_BDCSVD
      SVD(m,
          kernel->estimated_size(),
          enable_recon ? (Eigen::ComputeThinU | Eigen::ComputeThinV) : Eigen::EigenvaluesOnly),
#else
      XtX(std::min(m, kernel->estimated_size()), std::min(m, kernel->estimated_size())),
      eig(std::min(m, kernel->estimated_size())),
#endif
      s(std::min(m, kernel->estimated_size())),
      exports(exports) {
}

template <typename F>
Estimate<F>::Estimate(const Estimate<F> &that)
    : m(that.m),
      subsample(that.subsample),
      kernel(that.kernel),
      estimator(that.estimator),
      preconditioner_rank(that.preconditioner_rank),
      enable_recon(that.enable_recon),
      X(m, kernel->estimated_size()),
#ifndef DWIDENOISE2_USE_BDCSVD
      XtX(std::min(m, kernel->estimated_size()), std::min(m, kernel->estimated_size())),
      eig(std::min(m, kernel->estimated_size())),
#endif
      s(std::min(m, kernel->estimated_size())),
      exports(that.exports) {
}

template <typename F> void Estimate<F>::operator()(Image<F> &dwi) {

  // There are two options here for looping in the presence of subsampling:
  // 1. Loop over the input image
  //    Skip voxels that don't lie at the centre of a patch
  //    Have to transform input image voxel indices to subsampled image voxel indices for some optional outputs
  // 2. Loop over the subsampled image
  //    In some use cases there may not be any image created that conforms to this voxel grid
  //    Have to transform the subsampled voxel index into an input image voxel index for the centre of the patch
  // Going to go with 1. for now, as for 2. may not have a suitable image over which to loop
  Kernel::Voxel::index_type voxel({dwi.index(0), dwi.index(1), dwi.index(2)});
  if (!subsample->process(voxel))
    return;

  // Load list of voxels from which to import data
  patch = (*kernel)(voxel);
  const ssize_t n = patch.voxels.size();
  const ssize_t r = std::min(m, n);

  // Expand local storage if necessary
  if (n > X.cols()) {
    DEBUG("Expanding data matrix storage from " + str(m) + "x" + str(X.cols()) + " to " + str(m) + "x" + str(n));
    X.resize(m, n);
  }
#ifndef DWIDENOISE2_USE_BDCSVD
  if (r > XtX.cols()) {
    DEBUG("Expanding decomposition matrix storage from " + str(X.rows()) + " to " + str(r));
    XtX.resize(r, r);
  }
#endif
  if (r > s.size()) {
    DEBUG("Expanding eigenvalue storage from " + str(s.size()) + " to " + str(r));
    s.resize(r);
  }

  // Fill matrices with NaN when in debug mode;
  //   make sure results from one voxel are not creeping into another
  //   due to use of block oberations to prevent memory re-allocation
  //   in the presence of variation in kernel sizes
#ifndef NDEBUG
  X.fill(std::numeric_limits<F>::signaling_NaN());
#ifndef DWIDENOISE2_USE_BDCSVD
  XtX.fill(std::numeric_limits<F>::signaling_NaN());
#endif
  s.fill(std::numeric_limits<default_type>::signaling_NaN());
#endif

  load_data(dwi);
  assert(X.leftCols(n).allFinite());

  // Compute Eigendecomposition
#ifdef DWIDENOISE2_USE_BDCSVD
  SVD.compute(X.leftCols(n), enable_recon ? (Eigen::ComputeThinU | Eigen::ComputeThinV) : Eigen::EigenvaluesOnly);
  bool successful_decomposition = SVD.info() == Eigen::Success;
  if (successful_decomposition) {
    // eigenvalues sorted in increasing order:
    s.head(r) = SVD.singularValues().array().reverse().square().template cast<double>();
#else
  if (m <= n)
    XtX.topLeftCorner(r, r).template triangularView<Eigen::Lower>() = X.leftCols(n) * X.leftCols(n).adjoint();
  else
    XtX.topLeftCorner(r, r).template triangularView<Eigen::Lower>() = X.leftCols(n).adjoint() * X.leftCols(n);
  eig.compute(XtX.topLeftCorner(r, r), enable_recon ? Eigen::ComputeEigenvectors : Eigen::EigenvaluesOnly);
  bool successful_decomposition = eig.info() == Eigen::Success;
  if (successful_decomposition) {
    // eigenvalues sorted in increasing order:
    s.head(r) = eig.eigenvalues().template cast<double>();
#endif

    // Threshold determination, possibly via Marchenko-Pastur
    threshold = (*estimator)(s.head(r), m, n, preconditioner_rank, patch.centre_realspace);
  } else {
    s.head(r).fill(std::numeric_limits<double>::signaling_NaN());
    threshold = Estimator::Result();
  }

  // Store additional output maps if requested
  auto ss_index = subsample->in2ss(voxel);
  if (exports.noise_out.valid()) {
    assign_pos_of(ss_index).to(exports.noise_out);
    exports.noise_out.value() = bool(threshold)                                //
                                    ? float(std::sqrt(threshold.sigma2))       //
                                    : std::numeric_limits<float>::quiet_NaN(); //
  }
  if (exports.lamplus.valid()) {
    assign_pos_of(ss_index).to(exports.lamplus);
    exports.lamplus.value() = threshold.lamplus;
  }
  if (exports.rank_pcanonzero.valid()) {
    assign_pos_of(ss_index).to(exports.rank_pcanonzero);
    exports.rank_pcanonzero.value() = rank_nonzero(m, n, preconditioner_rank);
  }
  if (exports.rank_input.valid()) {
    assign_pos_of(ss_index).to(exports.rank_input);
    if (!successful_decomposition)
      exports.rank_input.value() = 0;
    else if (bool(threshold))
      exports.rank_input.value() = r - threshold.cutoff_p;
    else
      exports.rank_input.value() = r;
  }
  if (exports.max_dist.valid()) {
    assign_pos_of(ss_index).to(exports.max_dist);
    exports.max_dist.value() = patch.max_distance;
  }
  if (exports.voxelcount.valid()) {
    assign_pos_of(ss_index).to(exports.voxelcount);
    exports.voxelcount.value() = n;
  }
  if (exports.patchcount.valid()) {
    std::lock_guard<std::mutex> lock(Estimate<F>::mutex);
    for (const auto &v : patch.voxels) {
      assign_pos_of(v.index).to(exports.patchcount);
      exports.patchcount.value() = exports.patchcount.value() + 1;
    }
  }
  if (exports.saving_eigenspectra()) {
    std::lock_guard<std::mutex> lock(Estimate<F>::mutex);
    exports.add_eigenspectrum(s);
  }
}

template <typename F> void Estimate<F>::load_data(Image<F> &image) {
  const Kernel::Voxel::index_type pos({image.index(0), image.index(1), image.index(2)});
  for (ssize_t i = 0; i != patch.voxels.size(); ++i) {
    assign_pos_of(patch.voxels[i].index, 0, 3).to(image);
    X.col(i) = image.row(3);
  }
  assign_pos_of(pos, 0, 3).to(image);
}

template class Estimate<float>;
template class Estimate<cfloat>;
template class Estimate<double>;
template class Estimate<cdouble>;

} // namespace MR::Denoise
