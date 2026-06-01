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

#include "registration/warp/extrapolate.h"

#include <Eigen/Core>
#include <gtest/gtest.h>

#include <vector>

using namespace MR;
using namespace MR::Registration::Warp;

namespace {

//! voxel-index offsets filling a Chebyshev cube of the given radius, excluding the centre
std::vector<Eigen::Array<ssize_t, 3, 1>> cube_offsets(const ssize_t radius) {
  std::vector<Eigen::Array<ssize_t, 3, 1>> offsets;
  for (ssize_t dz = -radius; dz <= radius; ++dz)
    for (ssize_t dy = -radius; dy <= radius; ++dy)
      for (ssize_t dx = -radius; dx <= radius; ++dx)
        if (dx != 0 || dy != 0 || dz != 0)
          offsets.emplace_back(dx, dy, dz);
  return offsets;
}

//! evaluate an affine map value = b + A * offset for every offset
Eigen::Matrix<double, Eigen::Dynamic, 3> affine_values(const std::vector<Eigen::Array<ssize_t, 3, 1>> &offsets,
                                                       const Eigen::Vector3d &b,
                                                       const Eigen::Matrix3d &A) {
  Eigen::Matrix<double, Eigen::Dynamic, 3> values(static_cast<ssize_t>(offsets.size()), 3);
  for (ssize_t i = 0; i != values.rows(); ++i) {
    const Eigen::Vector3d o = offsets[i].cast<double>().matrix();
    values.row(i) = (b + A * o).transpose();
  }
  return values;
}

} // namespace

// An affine field is reproduced exactly: the fitted value at the centre equals
//   the constant term, regardless of whether the adaptive policy lands on the
//   quadratic or the affine model (an affine field lies in the span of both).
TEST(LocalPolynomialFit, AffineExactRecovery) {
  const auto offsets = cube_offsets(2);
  const Eigen::Vector3d b(1.5, -2.0, 3.25);
  Eigen::Matrix3d A;
  A << 0.5, -0.25, 0.1, 0.2, 0.4, -0.3, -0.1, 0.15, 0.6;
  const auto values = affine_values(offsets, b, A);
  const LocalFit fit = local_polynomial_fit(offsets, values, ExtrapolateDegree::Adaptive);
  EXPECT_TRUE(fit.kind == Fit::Quadratic || fit.kind == Fit::Affine);
  EXPECT_LT((fit.value - b).norm(), 1e-9);
}

// A quadratic field is reproduced exactly by the adaptive (quadratic) fit.
TEST(LocalPolynomialFit, QuadraticExactRecovery) {
  const auto offsets = cube_offsets(2);
  const Eigen::Vector3d b(-0.75, 4.0, 0.5);
  Eigen::Matrix<double, Eigen::Dynamic, 3> values(static_cast<ssize_t>(offsets.size()), 3);
  for (ssize_t i = 0; i != values.rows(); ++i) {
    const double x = static_cast<double>(offsets[i][0]);
    const double y = static_cast<double>(offsets[i][1]);
    const double z = static_cast<double>(offsets[i][2]);
    values(i, 0) = b[0] + 0.5 * x - 0.2 * y + 0.3 * x * x + 0.1 * y * z;
    values(i, 1) = b[1] - 0.4 * z + 0.25 * y * y - 0.15 * x * z;
    values(i, 2) = b[2] + 0.6 * x + 0.05 * z * z + 0.2 * x * y;
  }
  const LocalFit fit = local_polynomial_fit(offsets, values, ExtrapolateDegree::Adaptive);
  EXPECT_EQ(fit.kind, Fit::Quadratic);
  EXPECT_LT((fit.value - b).norm(), 1e-9);
}

// The affine-only policy never fits the quadratic terms, even given quadratic data.
TEST(LocalPolynomialFit, AffineOnlyPolicy) {
  const auto offsets = cube_offsets(2);
  const Eigen::Vector3d b(1.0, 1.0, 1.0);
  Eigen::Matrix3d A;
  A << 0.3, 0.0, 0.0, 0.0, 0.3, 0.0, 0.0, 0.0, 0.3;
  const auto values = affine_values(offsets, b, A);
  const LocalFit fit = local_polynomial_fit(offsets, values, ExtrapolateDegree::Affine);
  EXPECT_EQ(fit.kind, Fit::Affine);
  EXPECT_LT((fit.value - b).norm(), 1e-9);
}

// Too few samples to fit even an affine model: fall back to the sample mean.
TEST(LocalPolynomialFit, ConstantFallbackOnInsufficientSupport) {
  std::vector<Eigen::Array<ssize_t, 3, 1>> offsets;
  offsets.emplace_back(1, 0, 0);
  offsets.emplace_back(0, 1, 0);
  Eigen::Matrix<double, Eigen::Dynamic, 3> values(2, 3);
  values << 2.0, 4.0, 6.0, 4.0, 8.0, 12.0;
  const LocalFit fit = local_polynomial_fit(offsets, values, ExtrapolateDegree::Adaptive);
  EXPECT_EQ(fit.kind, Fit::Constant);
  EXPECT_LT((fit.value - Eigen::Vector3d(3.0, 6.0, 9.0)).norm(), 1e-12);
}

// Rank-deficient support (all samples collinear) drops below the affine model.
TEST(LocalPolynomialFit, ConstantFallbackOnRankDeficientSupport) {
  std::vector<Eigen::Array<ssize_t, 3, 1>> offsets;
  for (ssize_t dx = -2; dx <= 2; ++dx)
    if (dx != 0)
      offsets.emplace_back(dx, 0, 0); // collinear along x: y and z columns vanish
  Eigen::Matrix<double, Eigen::Dynamic, 3> values(static_cast<ssize_t>(offsets.size()), 3);
  for (ssize_t i = 0; i != values.rows(); ++i)
    values.row(i) = Eigen::RowVector3d(1.0, 2.0, 3.0);
  const LocalFit fit = local_polynomial_fit(offsets, values, ExtrapolateDegree::Adaptive);
  EXPECT_EQ(fit.kind, Fit::Constant);
  EXPECT_LT((fit.value - Eigen::Vector3d(1.0, 2.0, 3.0)).norm(), 1e-12);
}

// An empty sample set yields no fit.
TEST(LocalPolynomialFit, EmptyYieldsNone) {
  const std::vector<Eigen::Array<ssize_t, 3, 1>> offsets;
  const Eigen::Matrix<double, Eigen::Dynamic, 3> values(0, 3);
  const LocalFit fit = local_polynomial_fit(offsets, values, ExtrapolateDegree::Adaptive);
  EXPECT_EQ(fit.kind, Fit::None);
}
