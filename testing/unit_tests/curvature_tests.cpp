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

#include "gtest/gtest.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "types.h"

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/streamline.h"

namespace {

using MR::default_type;
using MR::DWI::Tractography::curvature;
using MR::DWI::Tractography::CurvatureConfig;
using MR::DWI::Tractography::CurvatureMethod;
using MR::DWI::Tractography::CurvatureScale;
using MR::DWI::Tractography::Streamline;
using point_type = Streamline<>::point_type;

constexpr default_type pi = 3.14159265358979323846;

// Sample a planar circle of radius R (mm) at a fixed arc-length step, over the given total angle.
Streamline<> circular_arc(const default_type radius, const default_type step_mm, const default_type total_angle) {
  Streamline<> tck;
  const default_type dtheta = step_mm / radius;
  for (default_type theta = 0.0; theta <= total_angle + 1.0e-9; theta += dtheta)
    tck.push_back(
        point_type(static_cast<float>(radius * std::cos(theta)), static_cast<float>(radius * std::sin(theta)), 0.0f));
  return tck;
}

// Sample a helix r(t) = (a cos t, a sin t, b t); constant curvature kappa = a / (a^2 + b^2).
Streamline<> helix(const default_type a, const default_type b, const default_type step_mm, const default_type total_t) {
  Streamline<> tck;
  // Arc-length speed of the helix parametrisation is sqrt(a^2 + b^2).
  const default_type dt = step_mm / std::sqrt(a * a + b * b);
  for (default_type t = 0.0; t <= total_t + 1.0e-9; t += dt)
    tck.push_back(point_type(
        static_cast<float>(a * std::cos(t)), static_cast<float>(a * std::sin(t)), static_cast<float>(b * t)));
  return tck;
}

// Median of the interior (non-endpoint) curvature estimates, where the truncated window is full.
default_type interior_median(const std::vector<default_type> &kappa) {
  if (kappa.size() < 5)
    return 0.0;
  // Drop a generous margin at each end so only well-supported interior vertices are assessed.
  const size_t margin = kappa.size() / 4;
  std::vector<default_type> interior(kappa.begin() + margin, kappa.end() - margin);
  std::sort(interior.begin(), interior.end());
  return interior[interior.size() / 2];
}

} // namespace

// Case 1: an analytic circular arc should recover kappa ~ 1/R at interior vertices.
TEST(CurvatureTest, CircularArcRecovery) {
  for (const default_type radius : {5.0, 20.0, 50.0}) {
    const Streamline<> tck = circular_arc(radius, 0.5, pi);
    const std::vector<default_type> kappa = curvature(tck);
    ASSERT_EQ(kappa.size(), tck.size());
    const default_type expected = 1.0 / radius;
    const default_type median = interior_median(kappa);
    EXPECT_NEAR(median, expected, 0.05 * expected) << "radius = " << radius;
  }
}

// Case 2: a genuine 3-D helix has constant curvature a / (a^2 + b^2) (a planar estimator would fail).
TEST(CurvatureTest, HelixRecovery) {
  const default_type a = 10.0;
  const default_type b = 4.0;
  const Streamline<> tck = helix(a, b, 0.5, 4.0 * pi);
  const std::vector<default_type> kappa = curvature(tck);
  ASSERT_EQ(kappa.size(), tck.size());
  const default_type expected = a / (a * a + b * b);
  const default_type median = interior_median(kappa);
  EXPECT_NEAR(median, expected, 0.05 * expected);
}

// Case 3: the recovered curvature must be (near) invariant to the sampling step.
TEST(CurvatureTest, ResamplingStepInvariance) {
  const default_type radius = 20.0;
  const default_type expected = 1.0 / radius;
  const default_type coarse = interior_median(curvature(circular_arc(radius, 1.5, pi)));
  const default_type fine = interior_median(curvature(circular_arc(radius, 0.5, pi)));
  EXPECT_NEAR(coarse, expected, 0.06 * expected);
  EXPECT_NEAR(fine, expected, 0.06 * expected);
  EXPECT_NEAR(coarse, fine, 0.06 * expected);
}

// Case 4: noise robustness; jittered vertices should not blow the median curvature up, and the
//   smoothing estimate should be far smaller than the raw finite-difference curvature.
TEST(CurvatureTest, NoiseRobustness) {
  const default_type radius = 40.0;
  const default_type step = 0.5;
  const default_type expected = 1.0 / radius;
  Streamline<> tck = circular_arc(radius, step, pi);

  std::mt19937 rng(12345);
  std::normal_distribution<default_type> jitter(0.0, step / 4.0);
  for (auto &v : tck) {
    v[0] += static_cast<float>(jitter(rng));
    v[1] += static_cast<float>(jitter(rng));
    v[2] += static_cast<float>(jitter(rng));
  }

  const std::vector<default_type> kappa = curvature(tck);
  const default_type median = interior_median(kappa);
  // The smoothed estimate stays within a tolerance band of the analytic value.
  EXPECT_NEAR(median, expected, 0.5 * expected);

  // Contrast: a raw three-point finite-difference curvature on the same jittered vertices is far
  //   larger (this is exactly the wiggle the estimator must suppress).
  default_type raw_sum = 0.0;
  size_t raw_count = 0;
  for (size_t i = 1; i + 1 < tck.size(); ++i) {
    const Eigen::Matrix<default_type, 3, 1> d1 = (tck[i] - tck[i - 1]).cast<default_type>();
    const Eigen::Matrix<default_type, 3, 1> d2 = (tck[i + 1] - tck[i]).cast<default_type>();
    const default_type l1 = d1.norm();
    const default_type l2 = d2.norm();
    if (l1 < 1.0e-6 || l2 < 1.0e-6)
      continue;
    const default_type dot = std::max<default_type>(-1.0, std::min<default_type>(1.0, d1.dot(d2) / (l1 * l2)));
    raw_sum += std::acos(dot) / (0.5 * (l1 + l2));
    ++raw_count;
  }
  const default_type raw_mean = raw_count ? (raw_sum / static_cast<default_type>(raw_count)) : 0.0;
  EXPECT_GT(raw_mean, 2.0 * median);
}

// Case 5a: fewer than three vertices is degenerate; every element must be exactly zero.
TEST(CurvatureTest, DegenerateTooShort) {
  for (const size_t n : {size_t(0), size_t(1), size_t(2)}) {
    Streamline<> tck;
    for (size_t i = 0; i != n; ++i)
      tck.push_back(point_type(static_cast<float>(i), 0.0f, 0.0f));
    const std::vector<default_type> kappa = curvature(tck);
    ASSERT_EQ(kappa.size(), n);
    for (const default_type k : kappa)
      EXPECT_EQ(k, 0.0);
  }
}

// Case 5b: a run of coincident (zero-length-step) vertices must yield finite, full-length output.
TEST(CurvatureTest, DegenerateCoincidentVertices) {
  const default_type radius = 20.0;
  Streamline<> tck = circular_arc(radius, 0.5, pi);
  // Inject repeated vertices into the interior.
  const point_type dup = tck[tck.size() / 2];
  tck.insert(tck.begin() + tck.size() / 2, dup);
  tck.insert(tck.begin() + tck.size() / 2, dup);

  const std::vector<default_type> kappa = curvature(tck);
  ASSERT_EQ(kappa.size(), tck.size());
  for (const default_type k : kappa) {
    EXPECT_TRUE(std::isfinite(k));
    EXPECT_GE(k, 0.0);
  }
  const default_type median = interior_median(kappa);
  EXPECT_NEAR(median, 1.0 / radius, 0.05 / radius);
}

// Case 6: a straight line has zero curvature everywhere.
TEST(CurvatureTest, StraightLine) {
  Streamline<> tck;
  for (size_t i = 0; i != 50; ++i)
    tck.push_back(point_type(static_cast<float>(i) * 0.7f, 0.0f, 0.0f));
  const std::vector<default_type> kappa = curvature(tck);
  ASSERT_EQ(kappa.size(), tck.size());
  for (const default_type k : kappa)
    EXPECT_NEAR(k, 0.0, 1.0e-6);
}

// The Gaussian-weighted variant shares the core and must also recover an analytic arc.
TEST(CurvatureTest, GaussianMethodCircularArc) {
  const default_type radius = 20.0;
  CurvatureConfig config;
  config.method = CurvatureMethod::GAUSSIAN_DERIV;
  const std::vector<default_type> kappa = curvature(circular_arc(radius, 0.5, pi), config);
  const default_type median = interior_median(kappa);
  EXPECT_NEAR(median, 1.0 / radius, 0.06 / radius);
}

// The FIXED-scale path is exercised and must likewise recover an analytic arc.
TEST(CurvatureTest, FixedScaleCircularArc) {
  const default_type radius = 30.0;
  CurvatureConfig config;
  config.scale = CurvatureScale::FIXED;
  config.fixed_scale_mm = 5.0;
  const std::vector<default_type> kappa = curvature(circular_arc(radius, 0.5, pi), config);
  const default_type median = interior_median(kappa);
  EXPECT_NEAR(median, 1.0 / radius, 0.06 / radius);
}
