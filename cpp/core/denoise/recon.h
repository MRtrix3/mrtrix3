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

#include <limits>
#include <map>
#include <memory>

#include <Eigen/Dense>

#include "denoise/estimate.h"
#include "denoise/estimator/base.h"
#include "denoise/exports.h"
#include "denoise/kernel/base.h"
#include "header.h"
#include "image.h"

namespace MR::Denoise {

template <typename F> class Recon : public Estimate<F> {

public:
  Recon(const Image<F> &image,
        std::shared_ptr<Subsample> subsample,
        std::shared_ptr<Kernel::Base> kernel,
        std::shared_ptr<Estimator::Base> estimator,
        filter_type filter,
        aggregator_type aggregator,
        Exports &exports,
        const ssize_t null_rank);

  void operator()(Image<F> &dwi, Image<F> &out);

protected:
  // Denoising configuration
  filter_type filter;
  aggregator_type aggregator;
  double gaussian_multiplier;

  // Reusable memory
  std::map<double, double> beta2lambdastar;
  vector_type w;
  typename Estimate<F>::MatrixType Xr;
};

} // namespace MR::Denoise
