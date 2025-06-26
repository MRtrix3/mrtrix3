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

#include "denoise/estimator/exp.h"

// Necessary for precompiler flags
#include "denoise/denoise.h"

namespace MR::Denoise::Estimator {

template <ssize_t version>
Result Exp<version>::operator()(const Eigen::VectorBlock<eigenvalues_type> s, //
                                const ssize_t m,                              //
                                const ssize_t n,                              //
                                const ssize_t rp,                             //
                                const Eigen::Vector3d & /*unused*/) const {   //
  assert(s.size() == std::min(m, n));
  const ssize_t qnz = dimlong_nonzero(m, n, rp);
  const ssize_t rz = rank_zero(m, n, rp);
#ifdef DWIDENOISE2_USE_BDCSVD
  const double lam_r = s[rz] / qnz;
#else
  const double lam_r = std::max(s[rz], 0.0) / qnz;
#endif
  double clam = 0.0;
  Result result;
  // Note that the paper utilised symbol "p" to refer to the number of signal components;
  //   here "p" is instead the index of the last noise component;
  //   therefore the number of noise compoments is (p + 1 - z)
  for (ssize_t p = rz; p < s.size(); ++p) {
#ifdef DWIDENOISE2_USE_BDCSVD
    const double lam = s[p] / qnz;
#else
    const double lam = std::max(s[p], 0.0) / qnz;
#endif
    clam += lam;
    double denominator = std::numeric_limits<double>::signaling_NaN();
    switch (version) {
    case 1:
      denominator = qnz;
      break;
    case 2:
      denominator = qnz - (s.size() - p - 1);
      break;
    default:
      assert(false);
    }
    const double gam = double(p + 1 - rz) / denominator;
    const double sigsq1 = clam / double(p + 1 - rz);
    const double sigsq2 = (lam - lam_r) / (4.0 * std::sqrt(gam));
    // sigsq2 > sigsq1 if signal else noise
    if (sigsq2 < sigsq1) {
      result.sigma2 = sigsq1;
      result.cutoff_p = p + 1;
      result.lamplus = lam;
    }
  }
  return result;
}

template class Exp<1>;
template class Exp<2>;

} // namespace MR::Denoise::Estimator
