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
#include "math/median.h"

namespace MR::Denoise::Estimator {

class Med : public Base {
public:
  Med() = default;
  Result operator()(const Eigen::VectorBlock<eigenvalues_type> s,
                    const ssize_t m,
                    const ssize_t n,
                    const ssize_t rp,
                    const Eigen::Vector3d & /*unused*/) const final {
    assert(s.size() == r);
    const ssize_t qnz = dimlong_nonzero(m, n, rp);
    const ssize_t rz = rank_zero(m, n, rp);
    const ssize_t rnz = rank_nonzero(m, n, rp);
    // Eigenvalues should already be sorted;
    //   no need to execute a sort for median calculation
    // Do however need to skip any components assumed to be zero based on preconditioning
    const double ymed = (s.size() - rz) & 1                                                            //
                            ? s[rz + (s.size() - rz) / 2]                                              //
                            : (0.5 * (s[rz + (s.size() - rz) / 2 - 1] + s[rz + (s.size() - rz) / 2])); //
    const ssize_t beta = double(rnz) / double(qnz);
    Result result;
    result.lamplus = ymed / (qnz * mu(beta));
    // It is not true that cutoff_p can be set as half of the rank
    //   based on this estimator being derived from the median eigenvalue;
    //   the upper bound of the MP distribution is influenced by the median eigenvalue,
    //   but it is not exactly that,
    //   and so a rank estimation process still needs to take place
    result.cutoff_p = rz;
    for (ssize_t p = rz; p != s.size(); ++p) {
      if (s[p] / qnz > result.lamplus)
        break;
      result.cutoff_p = p + 1;
    }
    // Calculate noise level based on MP distribution
#ifdef DWIDENOISE2_USE_BDCSVD
    result.sigma2 = 2.0 * s.segment(rz, result.cutoff_p - rz).sum() / (qnz * (result.cutoff_p + 1 - rz));
#else
    result.sigma2 = 2.0 *
                    s.segment(rz, result.cutoff_p - rz).unaryExpr([](double i) { return std::max(0.0, i); }).sum() /
                    (qnz * (result.cutoff_p + 1 - rz));
#endif
    return result;
  }

protected:
  // Coefficients as provided in Gavish and Donohue 2014
  // double omega(const double beta) const {
  //   const double betasq = Math::pow2(beta);
  //   return (0.56*beta*betasq - 0.95*betasq + 1.82*beta + 1.43);
  // }
  // Median of Marcenko-Pastur distribution
  // Third-order polynomial fit to data generated using Matlab code supplementary to Gavish and Donohue 2014
  double mu(const double beta) const {
    const double betasq = Math::pow2(beta);
    return ((-0.005882794526340723 * betasq * beta) //
            - (0.007508551496715836 * betasq)       //
            - (0.3338169644754149 * beta)           //
            + 1.0);                                 //
  }
};

} // namespace MR::Denoise::Estimator
