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

#ifndef __math_math_h__
#define __math_math_h__

#include <Eigen/Core>
#include <cmath>
#include <cstdlib>

#include "types.h"

namespace MR {
namespace Math {

/** @defgroup mathconstants Mathematical constants
  @{ */

constexpr double e = 2.71828182845904523536;
constexpr double pi = 3.14159265358979323846;
constexpr double pi_2 = pi / 2.0;
constexpr double pi_4 = pi / 4.0;
constexpr double sqrt2 = 1.41421356237309504880;
constexpr double sqrt1_2 = 1.0 / sqrt2;

/** @} */

/** @defgroup elfun Elementary Functions
  @{ */

template <typename T> inline constexpr T pow2(const T &v) { return v * v; }
template <typename T> inline constexpr T pow3(const T &v) { return pow2(v) * v; }
template <typename T> inline constexpr T pow4(const T &v) { return pow2(pow2(v)); }
template <typename T> inline constexpr T pow5(const T &v) { return pow4(v) * v; }
template <typename T> inline constexpr T pow6(const T &v) { return pow2(pow3(v)); }
template <typename T> inline constexpr T pow7(const T &v) { return pow6(v) * v; }
template <typename T> inline constexpr T pow8(const T &v) { return pow2(pow4(v)); }
template <typename T> inline constexpr T pow9(const T &v) { return pow8(v) * v; }
template <typename T> inline constexpr T pow10(const T &v) { return pow8(v) * pow2(v); }

template <typename I, typename T> inline constexpr I round(const T x) throw() { return static_cast<I>(std::round(x)); }
//! template function with cast to different type
/** example:
 * \code
 * float f = 21.412;
 * int x = floor<int> (f);
 * \endcode
 */
template <typename I, typename T> inline constexpr I floor(const T x) throw() { return static_cast<I>(std::floor(x)); }
//! template function with cast to different type
/** example:
 * \code
 * float f = 21.412;
 * int x = ceil<int> (f);
 * \endcode
 */
template <typename I, typename T> inline constexpr I ceil(const T x) throw() { return static_cast<I>(std::ceil(x)); }

/** @} */
} // namespace Math

//! convenience functions for SFINAE on std:: / Eigen containers
template <class Cont> class is_eigen_type {
  typedef char yes[1], no[2];
  template <typename C> static yes &test(typename C::Scalar);
  template <typename C> static no &test(...);

public:
  static const bool value = sizeof(test<Cont>(0)) == sizeof(yes);
};

//! Get the underlying scalar value type for both std:: containers and Eigen
template <class Cont, typename ReturnType = int> class container_value_type {
public:
  using type = typename Cont::value_type;
};
template <class Cont> class container_value_type<Cont, typename std::enable_if<is_eigen_type<Cont>::value, int>::type> {
public:
  using type = typename Cont::Scalar;
};

} // namespace MR

#endif
