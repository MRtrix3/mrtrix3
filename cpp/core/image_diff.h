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

#include <cmath>
#include <complex>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>

#include "algo/loop.h"
#include "image_helpers.h"

//! Machinery for selecting the appropriate arithmetic for comparing two image value types
namespace MR::ImageDiff {

//! whether a value type carries the concept of "not a number" (any floating-point, real or complex)
template <class ValueType>
struct is_floating
    : std::integral_constant<bool, std::is_floating_point<ValueType>::value || is_complex<ValueType>::value> {};

/*! \brief the value type in which to perform numerical comparison between two image value types
 *
 * The comparison is performed using the highest width / precision of the two inputs:
 * - complex floating-point if either input is complex;
 * - real floating-point if either (non-complex) input is floating-point;
 * - the wider integer (signed if either input is signed) if both inputs are integers.
 */
template <class T1, class T2, class = void> struct compute_type;

template <class T1, class T2>
struct compute_type<T1, T2, std::enable_if_t<is_complex<T1>::value || is_complex<T2>::value>> {
  using type = cdouble;
};

template <class T1, class T2>
struct compute_type<T1,
                    T2,
                    std::enable_if_t<!is_complex<T1>::value && !is_complex<T2>::value &&
                                     (std::is_floating_point<T1>::value || std::is_floating_point<T2>::value)>> {
  using type = double;
};

template <class T1, class T2>
struct compute_type<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_integral<T2>::value>> {
  using type = std::conditional_t<std::is_signed<T1>::value || std::is_signed<T2>::value, int64_t, uint64_t>;
};

template <class T1, class T2> using compute_t = typename compute_type<T1, T2>::type;

//! floating-point type used for relative (fractional) comparisons, which cannot be performed in integer arithmetic
template <class T1, class T2>
using float_compute_t = std::conditional_t<is_complex<T1>::value || is_complex<T2>::value, cdouble, double>;

//! test whether a real floating-point value is NaN
template <class ValueType>
inline std::enable_if_t<std::is_floating_point<ValueType>::value, bool> is_nan(const ValueType value) {
  return std::isnan(value);
}
//! test whether either component of a complex floating-point value is NaN
template <class ValueType>
inline std::enable_if_t<std::is_floating_point<ValueType>::value, bool> is_nan(const std::complex<ValueType> &value) {
  return std::isnan(value.real()) || std::isnan(value.imag());
}

//! whether two voxel values are inconsistent in the presence / absence of NaN; only relevant for floating-point inputs
template <class T1, class T2>
inline std::enable_if_t<is_floating<T1>::value || is_floating<T2>::value, bool> nan_mismatch(const T1 a, const T2 b) {
  using F = float_compute_t<T1, T2>;
  return is_nan(static_cast<F>(a)) != is_nan(static_cast<F>(b));
}
//! integer value types have no concept of NaN, hence can never be inconsistent
template <class T1, class T2>
inline std::enable_if_t<!is_floating<T1>::value && !is_floating<T2>::value, bool> nan_mismatch(const T1, const T2) {
  return false;
}

//! throw if two voxels are inconsistent in the presence / absence of NaN values
template <class T1, class T2>
inline void check_nan_match(const T1 a, const T2 b, const std::string_view name1, const std::string_view name2) {
  if (nan_mismatch(a, b))
    throw Exception("images \"" + std::string(name1) + "\" and \"" + std::string(name2) +
                    "\" do not match in locations of NaN values");
}

} // namespace MR::ImageDiff

namespace MR {

//! check image headers are the same (dimensions, spacing & transform)
template <class HeaderType1, class HeaderType2> inline void check_headers(HeaderType1 &in1, HeaderType2 &in2) {
  check_dimensions(in1, in2);
  for (size_t i = 0; i < in1.ndim(); ++i) {
    if (std::isfinite(in1.spacing(i)))
      if (std::fabs((in1.spacing(i) - in2.spacing(i)) / (in1.spacing(i) + in2.spacing(i))) > 1e-4)
        throw Exception("images \"" + in1.name() + "\" and \"" + in2.name() +
                        "\" do not have matching voxel spacings on axis " + str(i) + " (" + str(in1.spacing(i)) +
                        " vs " + str(in2.spacing(i)) + ")");
  }
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      if (std::fabs(in1.transform().matrix()(i, j) - in2.transform().matrix()(i, j)) > 0.001)
        throw Exception("images \"" + in1.name() + "\" and \"" + in2.name() +
                        "\" do not have matching header transforms:\n" + str(in1.transform().matrix()) + "\nvs:\n " +
                        str(in2.transform().matrix()) + ")");
    }
  }
}

//! check images are bitwise identical; by definition no comparison tolerance is applicable
template <class ImageType1, class ImageType2> inline void check_images_bitwise(ImageType1 &in1, ImageType2 &in2) {
  check_headers(in1, in2);
  ThreadedLoop(in1).run(
      [](const ImageType1 &a, const ImageType2 &b) {
        if (a.value() != b.value())
          throw Exception("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match in bitwise comparison (" +
                          str(a.value()) + " vs " + str(b.value()) + ")");
      },
      in1,
      in2);
}

//! check images are the same within a absolute tolerance
template <class ImageType1, class ImageType2>
inline void check_images_abs(ImageType1 &in1, ImageType2 &in2, const double tol = 0.0) {
  using C = ImageDiff::compute_t<typename ImageType1::value_type, typename ImageType2::value_type>;
  check_headers(in1, in2);
  ThreadedLoop(in1).run(
      [&tol](const ImageType1 &a, const ImageType2 &b) {
        const typename ImageType1::value_type va = a.value();
        const typename ImageType2::value_type vb = b.value();
        ImageDiff::check_nan_match(va, vb, a.name(), b.name());
        if (MR::abs(static_cast<C>(va) - static_cast<C>(vb)) > tol)
          throw Exception("images \"" + a.name() + "\" and \"" + b.name() +
                          "\" do not match within absolute precision of " + str(tol) + " (" + str(static_cast<C>(va)) +
                          " vs " + str(static_cast<C>(vb)) + ")");
      },
      in1,
      in2);
}

//! check images are the same within a fractional tolerance
template <class ImageType1, class ImageType2>
inline void check_images_frac(ImageType1 &in1, ImageType2 &in2, const double tol = 0.0) {
  using F = ImageDiff::float_compute_t<typename ImageType1::value_type, typename ImageType2::value_type>;
  check_headers(in1, in2);
  ThreadedLoop(in1).run(
      [&tol](const ImageType1 &a, const ImageType2 &b) {
        const typename ImageType1::value_type va = a.value();
        const typename ImageType2::value_type vb = b.value();
        ImageDiff::check_nan_match(va, vb, a.name(), b.name());
        if (MR::abs((static_cast<F>(va) - static_cast<F>(vb)) / (0.5 * (static_cast<F>(va) + static_cast<F>(vb)))) >
            tol)
          throw Exception("images \"" + a.name() + "\" and \"" + b.name() +
                          "\" do not match within fractional precision of " + str(tol) + " (" +
                          str(static_cast<F>(va)) + " vs " + str(static_cast<F>(vb)) + ")");
      },
      in1,
      in2);
}

//! check images are the same within a tolerance defined by a third image
template <class ImageType1, class ImageType2, class ImageTypeTol>
inline void check_images_tolimage(ImageType1 &in1, ImageType2 &in2, ImageTypeTol &tol) {
  using C = ImageDiff::compute_t<typename ImageType1::value_type, typename ImageType2::value_type>;
  check_headers(in1, in2);
  check_headers(in1, tol);
  ThreadedLoop(in1).run(
      [](const ImageType1 &a, const ImageType2 &b, const ImageTypeTol &t) {
        const typename ImageType1::value_type va = a.value();
        const typename ImageType2::value_type vb = b.value();
        ImageDiff::check_nan_match(va, vb, a.name(), b.name());
        if (MR::abs(static_cast<C>(va) - static_cast<C>(vb)) > t.value())
          throw Exception("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within precision of \"" +
                          t.name() + "\"" + " (" + str(static_cast<C>(va)) + " vs " + str(static_cast<C>(vb)) +
                          ", tolerance " + str(t.value()) + ")");
      },
      in1,
      in2,
      tol);
}

//! check images are the same within a fractional tolerance relative to the maximum value in the voxel
template <class ImageType1, class ImageType2>
inline void check_images_voxel(ImageType1 &in1, ImageType2 &in2, const double tol = 0.0) {
  using F = ImageDiff::float_compute_t<typename ImageType1::value_type, typename ImageType2::value_type>;
  auto func = [&tol](decltype(in1) &a, decltype(in2) &b) {
    double maxa = 0.0, maxb = 0.0;
    for (auto l = Loop(3)(a, b); l; ++l) {
      maxa = std::max(maxa, MR::abs(static_cast<F>(a.value())));
      maxb = std::max(maxb, MR::abs(static_cast<F>(b.value())));
    }
    const double threshold = tol * 0.5 * (maxa + maxb);
    for (auto l = Loop(3)(a, b); l; ++l) {
      const typename ImageType1::value_type va = a.value();
      const typename ImageType2::value_type vb = b.value();
      ImageDiff::check_nan_match(va, vb, a.name(), b.name());
      if (MR::abs(static_cast<F>(va) - static_cast<F>(vb)) > threshold)
        throw Exception("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within " + str(tol) +
                        " of maximal voxel value" + " (" + str(static_cast<F>(va)) + " vs " + str(static_cast<F>(vb)) +
                        ")");
    }
  };

  ThreadedLoop(in1, 0, 3).run(func, in1, in2);
}

//! check headers contain the same key-value entries
template <class HeaderType1, class HeaderType2>
inline void check_keyvals(const HeaderType1 &in1, const HeaderType2 &in2) {
  const static std::set<std::string> reserved{"command_history", "mrtrix_version", "project_version"};
  auto it1 = in1.keyval().cbegin();
  auto it2 = in2.keyval().cbegin();
  Exception errors;
  while (it1 != in1.keyval().end() || it2 != in2.keyval().end()) {
    while (it1 != in1.keyval().end() && reserved.find(it1->first) != reserved.end())
      ++it1;
    while (it2 != in2.keyval().end() && reserved.find(it2->first) != reserved.end())
      ++it2;

    if (it1 == in1.keyval().end() && it2 == in2.keyval().end())
      break;

    if (it1 == in1.keyval().end() || (it2 != in2.keyval().end() && it1->first > it2->first)) {
      errors.push_back("Key \"" + it2->first + "\" in image \"" + in2.name() + "\"" + //
                       " not present in \"" + in1.name() + "\"");                     //
      ++it2;
    } else if (it2 == in2.keyval().end() || (it1 != in1.keyval().end() && it1->first < it2->first)) {
      errors.push_back("Key \"" + it1->first + "\" in image \"" + in1.name() + "\"" + //
                       " not present in \"" + in2.name() + "\"");                     //
      ++it1;
    } else {
      if (it1->second != it2->second)
        errors.push_back("Key \"" + it1->first + "\" has different values between images");
      ++it1;
      ++it2;
    }
  }

  if (errors.num() > 0)
    throw errors;
}

//! check image headers are the same (dimensions, spacing & transform)
template <class HeaderType1, class HeaderType2> inline bool headers_match(HeaderType1 &in1, HeaderType2 &in2) {
  if (!dimensions_match(in1, in2))
    return false;
  if (!spacings_match(
          in1, in2, 1e-6)) // implicitly checked in voxel_grids_match_in_scanner_space but with different tolerance
    return false;
  if (!voxel_grids_match_in_scanner_space(in1, in2))
    return false;
  return true;
}

//! check images are the same within a absolute tolerance
template <class ImageType1, class ImageType2>
inline bool images_match_abs(ImageType1 &in1, ImageType2 &in2, const double tol = 0.0) {
  using C = ImageDiff::compute_t<typename ImageType1::value_type, typename ImageType2::value_type>;
  if (!headers_match(in1, in2))
    return false;
  for (auto i = Loop(in1)(in1, in2); i; ++i) {
    const typename ImageType1::value_type va = in1.value();
    const typename ImageType2::value_type vb = in2.value();
    if (ImageDiff::nan_mismatch(va, vb))
      return false;
    if (MR::abs(static_cast<C>(va) - static_cast<C>(vb)) > tol)
      return false;
  }
  return true;
}

} // namespace MR
