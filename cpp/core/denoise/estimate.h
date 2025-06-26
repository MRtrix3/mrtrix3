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

#pragma once

// Need to import this first to get relevant precompiler definitions
#include "denoise/denoise.h"

#include <memory>
#include <mutex>

#include <Eigen/Dense>
#ifdef DWIDENOISE2_USE_BDCSVD
#include <Eigen/SVD>
#else
#include <Eigen/Eigenvalues>
#endif

#include "denoise/denoise.h"
#include "denoise/estimator/base.h"
#include "denoise/estimator/result.h"
#include "denoise/exports.h"
#include "denoise/kernel/base.h"
#include "denoise/kernel/data.h"
#include "denoise/kernel/voxel.h"
#include "denoise/subsample.h"
#include "header.h"
#include "image.h"
#include "transform.h"

namespace MR::Denoise {

template <typename F> class Estimate {

public:
  using MatrixType = Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic>;

  Estimate(const Image<F> &image,
           std::shared_ptr<Subsample> subsample,
           std::shared_ptr<Kernel::Base> kernel,
           std::shared_ptr<Estimator::Base> estimator,
           Exports &exports,
           const ssize_t preconditioner_rank = 0,
           const bool enable_recon = false);

  Estimate(const Estimate &);

  void operator()(Image<F> &dwi);

protected:
  const ssize_t m;

  // Denoising configuration
  std::shared_ptr<Subsample> subsample;
  std::shared_ptr<Kernel::Base> kernel;
  std::shared_ptr<Estimator::Base> estimator;
  ssize_t preconditioner_rank;
  bool enable_recon;

  // Reusable memory
  Kernel::Data patch;
  MatrixType X;
  // TODO For both BDCSVD and SelfAdjointEigenSolver,
  //   the template type is MatrixType,
  //   and it doesn't seem to be possible to define an Eigen::Block as this template type;
  //   as such, most likely in both circumstances it is actually constructing a MatrixType from Eigen::Block
  //   in order to construct the decomposition
  // What could conceivably be done instead,
  //   given that these matrices are relatively small
  //   and the number of unique patch sizes is small (though not necessarily one),
  //   would be to construct a std::map<> from patch size to PCA memory;
  //   each processing thread would allocate new memory for new patch sizes not yet encountered by it,
  //   but the total memory consumption should still be relatively small;
  //   note that "X" would be subsumed within such a mechanism also
#ifdef DWIDENOISE2_USE_BDCSVD
  Eigen::BDCSVD<MatrixType> SVD;
#else
  MatrixType XtX;
  Eigen::SelfAdjointEigenSolver<MatrixType> eig;
#endif
  eigenvalues_type s;
  Estimator::Result threshold;

  // Export images
  // Note: One instance created per thread,
  //   so that when possible output image data can be written without mutex-locking
  Exports exports;

  // Some data can only be written in a thread-safe manner
  static std::mutex mutex;

  void load_data(Image<F> &image);
};

template <typename F> std::mutex Estimate<F>::mutex;

} // namespace MR::Denoise
