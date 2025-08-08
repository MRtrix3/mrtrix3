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

namespace MR::Denoise::Estimator {

class Result {
public:
  Result()
      : cutoff_p(-1),
        sigma2(std::numeric_limits<double>::signaling_NaN()),
        lamplus(std::numeric_limits<double>::signaling_NaN()) {}
  operator bool() const { return cutoff_p >= 0 && std::isfinite(sigma2) && std::isfinite(lamplus); }
  bool operator!() const { return !bool(*this); }
  // From dwidenoise code / estimator::Exp :
  //   cutoff_p is the *number of noise components considered to be part of the MP distribution*
  ssize_t cutoff_p;
  double sigma2;
  double lamplus;
};

} // namespace MR::Denoise::Estimator
