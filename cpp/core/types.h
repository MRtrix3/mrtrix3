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

#include <cinttypes>
#include <complex>
#include <cstddef>
#include <deque>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#ifdef _WIN32
#ifdef _WIN64
#define PRI_SIZET PRIu64
#else
#define PRI_SIZET PRIu32
#endif
#else
#define PRI_SIZET "zu" // check_syntax off
#endif

namespace MR::Helper {
template <class ImageType> class ConstRow;
template <class ImageType> class Row;
} // namespace MR::Helper
#define EIGEN_DENSEBASE_PLUGIN "eigen_plugins/dense_base.h"  // check_syntax off
#define EIGEN_MATRIXBASE_PLUGIN "eigen_plugins/dense_base.h" // check_syntax off
#define EIGEN_ARRAYBASE_PLUGIN "eigen_plugins/dense_base.h"  // check_syntax off
#define EIGEN_MATRIX_PLUGIN "eigen_plugins/matrix.h"         // check_syntax off
#define EIGEN_ARRAY_PLUGIN "eigen_plugins/array.h"           // check_syntax off
#include <Eigen/Geometry>
#ifdef EIGEN_HAS_OPENMP
#undef EIGEN_HAS_OPENMP
#endif

/*! \defgroup VLA Variable-length array macros
 *
 * The reason for defining these macros at all is that VLAs are not part of the
 * C++ standard, and not available on all compilers. Regardless of the
 * availability of VLAs, they should be avoided if possible since they run the
 * risk of overrunning the stack if the length of the array is large, or if the
 * function is called recursively. They can be used safely in cases where the
 * size of the array is expected to be small, and the function will not be
 * called recursively, and in these cases may avoid the overhead of allocation
 * that might be incurred by the use of e.g. a vector.
 */

//! \{

/*! \def VLA
 * define a variable-length array (VLA) if supported by the compiler, or a
 * vector otherwise. This may have performance implications in the latter
 * case if this forms part of a tight loop.
 * \sa VLA_MAX
 */

/*! \def VLA_MAX
 * define a variable-length array if supported by the compiler, or a
 * fixed-length array of size \a max  otherwise. This may have performance
 * implications in the latter case if this forms part of a tight loop.
 * \note this should not be used in recursive functions, unless the maximum
 * number of calls is known to be small. Large amounts of recursion will run
 * the risk of overrunning the stack.
 * \sa VLA
 */

#ifdef MRTRIX_NO_VLA
#define VLA(name, type, num)                                                                                           \
  std::vector<type> __vla__##name(num);                                                                                \
  type *name = &__vla__##name[0]
#define VLA_MAX(name, type, num, max) type name[max]
#else
#define VLA(name, type, num) type name[num]
#define VLA_MAX(name, type, num, max) type name[num]
#endif

/*! \def NON_POD_VLA
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a vector otherwise. This may have performance implications in the
 * latter case if this forms part of a tight loop.
 * \sa VLA_MAX
 */

/*! \def NON_POD_VLA_MAX
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a fixed-length array of size \a max  otherwise. This may have performance
 * implications in the latter case if this forms part of a tight loop.
 * \note this should not be used in recursive functions, unless the maximum
 * number of calls is known to be small. Large amounts of recursion will run
 * the risk of overrunning the stack.
 * \sa VLA
 */

#ifdef MRTRIX_NO_NON_POD_VLA
#define NON_POD_VLA(name, type, num)                                                                                   \
  std::vector<type> __vla__##name(num);                                                                                \
  type *name = &__vla__##name[0]
#define NON_POD_VLA_MAX(name, type, num, max) type name[max]
#else
#define NON_POD_VLA(name, type, num) type name[num]
#define NON_POD_VLA_MAX(name, type, num, max) type name[num]
#endif

//! \}

#ifdef NDEBUG
#define FORCE_INLINE inline __attribute__((always_inline))
#else // don't force inlining in debug mode, so we can get more informative backtraces
#define FORCE_INLINE inline
#endif

namespace MR {

#ifdef MRTRIX_MAX_ALIGN_T_NOT_DEFINED
#ifdef MRTRIX_STD_MAX_ALIGN_T_NOT_DEFINED
// needed for clang 3.4:
using __max_align_t = struct {
  long long __clang_max_align_nonce1 __attribute__((__aligned__(__alignof__(long long))));
  long double __clang_max_align_nonce2 __attribute__((__aligned__(__alignof__(long double))));
};
constexpr size_t malloc_align = alignof(__max_align_t);
#else
constexpr size_t malloc_align = alignof(std::max_align_t);
#endif
#else
constexpr size_t malloc_align = alignof(::max_align_t);
#endif

using float32 = float;
using float64 = double;
using cdouble = std::complex<double>;
using cfloat = std::complex<float>;

template <typename T> struct container_cast : public T {
  template <typename U> container_cast(const U &x) : T(x.begin(), x.end()) {}
};

//! the default type used throughout MRtrix
using default_type = double;

constexpr default_type NaN = std::numeric_limits<default_type>::quiet_NaN();
constexpr float NaNF = std::numeric_limits<float>::quiet_NaN();
constexpr default_type Inf = std::numeric_limits<default_type>::infinity();
constexpr float InfF = std::numeric_limits<float>::infinity();

//! the type for the affine transform of an image:
using transform_type = Eigen::Transform<default_type, 3, Eigen::AffineCompact>;

//! used in various places for storing key-value pairs
using KeyValues = std::map<std::string, std::string>;

//! check whether type is complex:
template <class ValueType> struct is_complex : std::false_type {};
template <class ValueType> struct is_complex<std::complex<ValueType>> : std::true_type {};

//! check whether type is compatible with MRtrix3's file IO backend:
template <class ValueType>
struct is_data_type
    : std::integral_constant<bool, std::is_arithmetic<ValueType>::value || is_complex<ValueType>::value> {};

// required to allow use of abs() call on unsigned integers in template
// functions, etc, since the standard labels such calls ill-formed:
// http://en.cppreference.com/w/cpp/numeric/math/abs
template <typename X>
inline constexpr typename std::enable_if<std::is_arithmetic<X>::value && std::is_unsigned<X>::value, X>::type abs(X x) {
  return x;
}
template <typename X>
inline constexpr typename std::enable_if<std::is_floating_point<X>::value, X>::type abs(const std::complex<X> &x) {
  return std::abs(x);
}
template <typename X>
inline constexpr typename std::enable_if<std::is_arithmetic<X>::value && !std::is_unsigned<X>::value, X>::type
abs(X x) {
  return std::abs(x);
}

template <class ValueType> struct is_string_type : std::false_type {};
template <> struct is_string_type<std::string> : std::true_type {};
template <> struct is_string_type<std::string_view> : std::true_type {};
template <> struct is_string_type<const char *const> : std::true_type {}; // check_syntax off
template <> struct is_string_type<const char *> : std::true_type {};      // check_syntax off

//! convenience functions for SFINAE on std:: / Eigen containers
template <class Cont> class is_eigen_type {
  typedef char yes[1], no[2]; // check_syntax off
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

namespace std {

template <class T> inline ostream &operator<<(ostream &stream, const std::vector<T> &V) {
  stream << "[ ";
  for (size_t n = 0; n < V.size(); n++)
    stream << V[n] << " ";
  stream << "]";
  return stream;
}

template <class T, std::size_t N> inline ostream &operator<<(ostream &stream, const array<T, N> &V) {
  stream << "[ ";
  for (size_t n = 0; n < N; n++)
    stream << V[n] << " ";
  stream << "]";
  return stream;
}

} // namespace std

namespace fmt {
// std::vector and std::array are formatted by fmtlib's native range formatter (<fmt/ranges.h>),
// yielding "[1, 2, 3]".

//! Disable fmtlib's native range formatter for Eigen matrix/array expressions, so the dedicated
//! formatter below (which mirrors Eigen's own layout) applies unambiguously rather than colliding
//! with the generic range formatter that would otherwise treat an Eigen object as a range.
template <typename Derived>
struct range_format_kind<
    Derived,
    char,
    std::enable_if_t<
        MR::is_eigen_type<std::remove_cv_t<Derived>>::value &&
        (std::is_same_v<typename Eigen::internal::traits<std::remove_cv_t<Derived>>::XprKind, Eigen::MatrixXpr> ||
         std::is_same_v<typename Eigen::internal::traits<std::remove_cv_t<Derived>>::XprKind, Eigen::ArrayXpr>)>>
    : std::integral_constant<range_format, range_format::disabled> {};

//! Format an Eigen matrix/array by delegating to Eigen's own ostream insertion operator.
//! Spec grammar: an optional single 'T' selects "display the transpose": the expression is
//! transposed before being emitted and a trailing "^T" suffix is appended. Without the spec
//! the expression is emitted as-is (a column vector therefore renders multi-line, matching
//! Eigen's native ostream layout); the spec is the only way to obtain the transposed
//! single-line form. No other spec characters are accepted (use Eigen::IOFormat for richer
//! control via Eigen::WithFormat).
template <typename Derived>
struct formatter<
    Derived,
    char,
    std::enable_if_t<
        MR::is_eigen_type<std::remove_cv_t<Derived>>::value &&
        (std::is_same_v<typename Eigen::internal::traits<std::remove_cv_t<Derived>>::XprKind, Eigen::MatrixXpr> ||
         std::is_same_v<typename Eigen::internal::traits<std::remove_cv_t<Derived>>::XprKind, Eigen::ArrayXpr>)>> {
  bool transpose = false;
  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin();
    const auto end = ctx.end();
    if (it != end && *it == 'T') {
      transpose = true;
      ++it;
    }
    if (it != end && *it != '}')
      throw format_error("invalid format spec for Eigen matrix/array (expected optional 'T' then '}')");
    return it;
  }
  template <typename FormatContext> auto format(const Derived &m, FormatContext &ctx) const {
    if (transpose)
      return fmt::format_to(ctx.out(), "{}^T", fmt::streamed(m.transpose()));
    return fmt::format_to(ctx.out(), "{}", fmt::streamed(m));
  }
};
template <typename T> struct formatter<Eigen::Transform<T, 3, Eigen::AffineCompact>> {
  fmt::formatter<Eigen::Matrix<T, 3, 4>> matrix_formatter;
  constexpr auto parse(format_parse_context &ctx) { return matrix_formatter.parse(ctx); }
  template <typename FormatContext>
  auto format(const Eigen::Transform<T, 3, Eigen::AffineCompact> &t, FormatContext &ctx) const {
    return matrix_formatter.format(t.matrix(), ctx);
  }
};
//! Format an Eigen expression carrying a custom Eigen::IOFormat (e.g. matrix.format(iofmt))
//! by delegating to its ostream insertion operator, preserving the requested formatting.
template <typename T> struct formatter<Eigen::WithFormat<T>> : ostream_formatter {};
} // namespace fmt
