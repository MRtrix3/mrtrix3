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

#include <Eigen/Core>
#include <fmt/format.h>

#include "math/math.h"

namespace fmt {

// Formatter for all Eigen dense expressions (Matrix, Array, Transpose, Block, Map, etc.)
// Uses is_eigen_type (has ::Scalar) plus XprKind to cover all dense Eigen expression templates,
// including wrapper types (Block, VectorBlock, Transpose) whose CRTP DenseBase parameter
// differs from the outermost type.
template <typename Derived>
struct formatter<
    Derived,
    char,
    std::enable_if_t<
        MR::is_eigen_type<std::remove_cv_t<Derived>>::value &&
        (std::is_same_v<typename Eigen::internal::traits<std::remove_cv_t<Derived>>::XprKind, Eigen::MatrixXpr> ||
         std::is_same_v<typename Eigen::internal::traits<std::remove_cv_t<Derived>>::XprKind, Eigen::ArrayXpr>)>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  template <typename FormatContext> auto format(const Derived &m, FormatContext &ctx) const {
    auto out = ctx.out();
    if (m.rows() == 1) {
      format_to(out, "[ ");
      for (Eigen::Index i = 0; i < m.cols(); ++i)
        format_to(out, "{} ", m.coeff(0, i));
      return format_to(out, "]");
    } else if (m.cols() == 1) {
      format_to(out, "[ ");
      for (Eigen::Index i = 0; i < m.rows(); ++i)
        format_to(out, "{} ", m.coeff(i, 0));
      return format_to(out, "]^T");
    } else {
      format_to(out, "\n[ ");
      for (Eigen::Index i = 0; i < m.rows(); ++i) {
        if (i > 0)
          format_to(out, "\n");
        for (Eigen::Index j = 0; j < m.cols(); ++j)
          format_to(out, "{} ", m.coeff(i, j));
      }
      return format_to(out, "]");
    }
  }
};

template <typename T> struct formatter<Eigen::Transform<T, 3, Eigen::AffineCompact>> {
  fmt::formatter<Eigen::Matrix<T, 3, 4>> matrix_formatter;
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const Eigen::Transform<T, 3, Eigen::AffineCompact> &t, FormatContext &ctx) const {
    return matrix_formatter.format(t.matrix(), ctx);
  }
};

} // namespace fmt

//! @}
