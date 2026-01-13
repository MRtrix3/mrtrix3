/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <Eigen/Dense>

#include "math/chebyshev.h"

namespace MR::Math::Bessel {

extern const Eigen::Array<double, 12, 1> coef_aI0;
extern const Eigen::Array<double, 21, 1> coef_bI0;
extern const Eigen::Array<double, 22, 1> coef_cI0;

extern const Eigen::Array<double, 11, 1> coef_aI1;
extern const Eigen::Array<double, 21, 1> coef_bI1;
extern const Eigen::Array<double, 22, 1> coef_cI1;

//* Compute the scaled regular modified cylindrical Bessel function of zeroth order exp(-|x|) I_0(x). */
/** Implementation based on the GSL (http://www.gnu.org/software/gsl/) */
template <typename T> inline T I0_scaled(const T x) {
  assert(x >= 0.0);
  if (x * x < 4.0 * std::numeric_limits<T>::epsilon())
    return (1.0 - x);
  if (x <= 3.0)
    return (exp(-x) * (2.75 + Chebyshev::eval(coef_aI0, 11, T(-1), T(1), x * x / 4.5 - 1.0)));
  if (x <= 8.0)
    return ((0.375 + Chebyshev::eval(coef_bI0, (sizeof(T) > 4 ? 20 : 13), T(-1), T(1), (48.0 / x - 11.0) / 5.0)) /
            sqrt(x));
  return ((0.375 + Chebyshev::eval(coef_cI0, (sizeof(T) > 4 ? 21 : 11), T(-1), T(1), 16.0 / x - 1.0)) / sqrt(x));
}

//* Compute the scaled regular modified cylindrical Bessel function of first order exp(-|x|) I_1(x). */
/** Implementation based on the GSL (http://www.gnu.org/software/gsl/) */
template <typename T> inline T I1_scaled(const T x) {
  assert(x >= 0.0);
  if (x == 0.0)
    return (0.0);
  if (x * x < 8.0 * std::numeric_limits<T>::epsilon())
    return (0.5 * x);
  if (x <= 3.0)
    return (x * exp(-x) * (0.875 + Chebyshev::eval(coef_aI1, 10, T(-1), T(1), x * x / 4.5 - 1.0)));
  if (x <= 8.0) {
    T b =
        (0.375 + Chebyshev::eval(coef_bI1, (sizeof(T) > 4 ? 20 : 11), T(-1), T(1), (48.0 / x - 11.0) / 5.0)) / sqrt(x);
    return (x > 0.0 ? b : -b);
  }
  T b = (0.375 + Chebyshev::eval(coef_cI1, (sizeof(T) > 4 ? 21 : 9), T(-1), T(1), 16.0 / x - 1.0)) / sqrt(x);
  return (x > 0.0 ? b : -b);
}

} // namespace MR::Math::Bessel
