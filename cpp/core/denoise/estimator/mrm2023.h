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

#include "denoise/denoise.h"
#include "denoise/estimator/base.h"
#include "denoise/estimator/result.h"
#include "math/math.h"

namespace MR::Denoise::Estimator {

class MRM2023 : public Base {
public:
  MRM2023() = default;
  Result operator()(const Eigen::VectorBlock<eigenvalues_type> s,     //
                    const ssize_t m,                                  //
                    const ssize_t n,                                  //
                    const ssize_t rp,                                 //
                    const Eigen::Vector3d & /*unused*/) const final { //
    // MRM2023 article suggests that mprime should subtract z
    //   since it refers to "non-zero singular values";
    //   possible that this is the case for all estimators
    assert(s.size() == std::min(m, n));
    const ssize_t rz = rank_zero(m, n, rp);
    const ssize_t mprime = rank_nonzero(m, n, rp);
    const ssize_t nprime = dimlong_nonzero(m, n, rp);
    const double sigmasq_to_lamplus = Math::pow2(std::sqrt(nprime) + std::sqrt(mprime));
    double clam = s.segment(rz, mprime).sum() / nprime;
    // Unlike Exp# code,
    //   MRM2023 article uses p to index number of signal components,
    //   and here doing a direct translation of the manuscript content to code
    double lamplusprev = -std::numeric_limits<double>::infinity();
    Result result;
    for (ssize_t p = 0; p < mprime; ++p) {
      const ssize_t i = s.size() - 1 - p;
#ifdef DWIDENOISE2_USE_BDCSVD
      const double lam = s[i] / nprime;
#else
      const double lam = std::max(s[i], 0.0) / nprime;
#endif
      if (lam < lamplusprev)
        return result;
      clam -= lam;
      const double sigmasq = clam / ((mprime - p) * (nprime - p));
      lamplusprev = sigmasq * sigmasq_to_lamplus;
      result.cutoff_p = i;
      result.sigma2 = sigmasq;
      result.lamplus = lamplusprev;
    }
    return result;
  }
};

} // namespace MR::Denoise::Estimator
