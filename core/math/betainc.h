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

#ifndef __math_betainc_h__
#define __math_betainc_h__

#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
#include <unsupported/Eigen/SpecialFunctions>
#endif

#ifndef MRTRIX_HAVE_LGAMMA_R
#include <mutex>
#endif

#include "types.h"

namespace MR::Math {

#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS

// Compute the *regularised* incomplete beta function using
//   the incomplete beta function as provided in Eigen 3.3 and above
template <typename ArgADerived, typename ArgBDerived, typename ArgXDerived>
inline const Eigen::Array<typename ArgXDerived::Scalar, Eigen::Dynamic, Eigen::Dynamic>
betaincreg(const Eigen::ArrayBase<ArgADerived> &a,
           const Eigen::ArrayBase<ArgBDerived> &b,
           const Eigen::ArrayBase<ArgXDerived> &x) {
  Eigen::Array<typename ArgXDerived::Scalar, Eigen::Dynamic, Eigen::Dynamic> ones(x);
  ones.fill(typename ArgXDerived::Scalar(1));
  return (Eigen::betainc(a, b, x) / Eigen::betainc(a, b, ones)).eval();
};

#endif

default_type betaincreg(const default_type a, const default_type b, const default_type x);

#ifndef MRTRIX_HAVE_LGAMMA_R
extern std::mutex mutex_lgamma;
#endif

} // namespace MR::Math

#endif
