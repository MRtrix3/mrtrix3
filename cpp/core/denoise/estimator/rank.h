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

#include "denoise/estimator/base.h"
#include "denoise/estimator/result.h"

namespace MR::Denoise::Estimator {

class Rank : public Base {
public:
  Rank(const ssize_t r) : rank(r) {}
  Result operator()(const Eigen::VectorBlock<eigenvalues_type> s,     //
                    const ssize_t m,                                  //
                    const ssize_t n,                                  //
                    const ssize_t rp,                                 //
                    const Eigen::Vector3d & /*unused*/) const final { //
    assert(s.size() == r);
    const ssize_t rz = rank_zero(m, n, rp);
    const ssize_t rnz = rank_nonzero(m, n, rp);
    const ssize_t qnz = dimlong_nonzero(m, n, rp);
    Result result;
    // Bear in mind that any assumed-zero singular values "rz" due to preconditioning "rp"
    //   must be assumed to contribute to the rank
    if (rnz == rank) {
      // All components contribute (even the assumed-zero ones)
      result.cutoff_p = 0;
      result.lamplus = 0.0;
      result.sigma2 = 0.0;
    } else if (rnz > rank) {
      result.cutoff_p = s.size() - (rank - rz);
      result.sigma2 = s.segment(rz, result.cutoff_p - rz).sum() / (qnz * (result.cutoff_p + 1 - rz));
      result.lamplus = s[result.cutoff_p - 1] / qnz;
    } // If requested rank is greater than available rank, leave "result" completely uninitialised
    return result;
  }

protected:
  const ssize_t rank;
};

} // namespace MR::Denoise::Estimator
