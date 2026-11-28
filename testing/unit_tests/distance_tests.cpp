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

#include <cmath>
#include <vector>

#include "types.h"

#include "dwi/tractography/distance.h"
#include "dwi/tractography/foot_point.h"
#include "dwi/tractography/spline.h"
#include "dwi/tractography/streamline.h"

namespace {

using MR::default_type;
using MR::DWI::Tractography::hausdorff;
using MR::DWI::Tractography::HausdorffConfig;
using MR::DWI::Tractography::HausdorffResult;
using MR::DWI::Tractography::SplineView;
using MR::DWI::Tractography::Streamline;
using MR::DWI::Tractography::FootPoint::nearest_point;
using MR::DWI::Tractography::FootPoint::vec3;
using point_type = Streamline<>::point_type;

constexpr default_type pi = 3.14159265358979323846;

// A straight line along x from 0 to length, sampled at the given step.
Streamline<> straight_line(const default_type length, const default_type step) {
  Streamline<> tck;
  for (default_type x = 0.0; x <= length + 1.0e-9; x += step)
    tck.push_back(point_type(static_cast<float>(x), 0.0f, 0.0f));
  return tck;
}

// A planar circular arc of radius R, sampled at a fixed arc-length step, over total_angle radians.
Streamline<> circular_arc(const default_type radius, const default_type step_mm, const default_type total_angle) {
  Streamline<> tck;
  const default_type dtheta = step_mm / radius;
  for (default_type theta = 0.0; theta <= total_angle + 1.0e-9; theta += dtheta)
    tck.push_back(
        point_type(static_cast<float>(radius * std::cos(theta)), static_cast<float>(radius * std::sin(theta)), 0.0f));
  return tck;
}

// Copy a streamline displaced by a constant offset vector.
Streamline<> offset_by(const Streamline<> &tck, const point_type &delta) {
  Streamline<> out;
  for (const auto &v : tck)
    out.push_back(v + delta);
  return out;
}

// Keep every k-th vertex (plus the last), i.e. a uniform decimation of the input.
Streamline<> decimate(const Streamline<> &tck, const size_t keep_every) {
  Streamline<> out;
  for (size_t i = 0; i < tck.size(); i += keep_every)
    out.push_back(tck[i]);
  if (out.back() != tck.back())
    out.push_back(tck.back());
  return out;
}

} // namespace

// Identical streamlines have zero Hausdorff distance.
TEST(HausdorffTest, IdenticalIsZero) {
  const Streamline<> tck = circular_arc(20.0, 0.5, pi);
  const HausdorffResult r = hausdorff(tck, tck);
  EXPECT_NEAR(r.distance, 0.0, 1.0e-4);
}

// A streamline against a constant parallel offset returns the offset magnitude.
TEST(HausdorffTest, ParallelOffset) {
  const Streamline<> a = straight_line(50.0, 1.0);
  // Offset perpendicular to the line so the nearest-point distance is exactly the offset.
  const point_type delta(0.0f, 3.0f, 0.0f);
  const Streamline<> b = offset_by(a, delta);
  const HausdorffResult r = hausdorff(a, b);
  EXPECT_NEAR(r.distance, 3.0, 1.0e-3);
}

// A curved streamline against a parallel offset (perpendicular to the plane) returns the offset.
TEST(HausdorffTest, CurvedParallelOffset) {
  const Streamline<> a = circular_arc(20.0, 0.5, pi);
  const point_type delta(0.0f, 0.0f, 2.0f); // out of the arc's plane: constant separation
  const Streamline<> b = offset_by(a, delta);
  const HausdorffResult r = hausdorff(a, b);
  EXPECT_NEAR(r.distance, 2.0, 5.0e-3);
}

// A decimated arc versus its original: error is bounded by the chord-vs-arc sag of the decimated
//   spline. The CR spline of the decimated vertices tracks the dense arc far better than the raw
//   chord, so a generous analytic bound (the chord sag) must comfortably hold.
TEST(HausdorffTest, DecimatedArcWithinBound) {
  const default_type radius = 30.0;
  const default_type step = 0.5;
  const Streamline<> dense = circular_arc(radius, step, pi);
  const size_t keep_every = 8;
  const Streamline<> coarse = decimate(dense, keep_every);

  const HausdorffResult r = hausdorff(dense, coarse);
  // Decimated chord length ~ keep_every*step; chord-vs-arc sag = L^2 / (8 R) is a loose upper bound
  // on the spline-vs-spline deviation (the CR spline interpolates the kept vertices, sagging less).
  const default_type chord = static_cast<default_type>(keep_every) * step;
  const default_type sag_bound = (chord * chord) / (8.0 * radius);
  EXPECT_GT(r.distance, 0.0);
  EXPECT_LT(r.distance, sag_bound);
}

// Regression guard for the foot-point overshoot on a high-curvature target. On a near-circular
//   spline the foot-point condition (Q-P(s)).P'(s)=0 is satisfied at BOTH the nearest point and the
//   diametrically opposite (farthest) point; the curvature-blind Gauss-Newton step, started far from
//   the true foot, used to leap clear across the curve to that far stationary point and clamp at an
//   endpoint, reporting a distance on the order of the loop diameter. The backtracking descent must
//   instead converge to the genuine nearest point even from an adversarial (near-antipodal) start.
TEST(FootPointTest, NoAntipodalOvershootOnLoop) {
  const default_type radius = 50.0;
  const default_type tension = 0.1;
  const Streamline<> circle = circular_arc(radius, 0.5, 2.0 * pi); // full turn
  const SplineView<> view(circle);
  const default_type s_max = static_cast<default_type>(circle.size() - 1);

  // Probe just outside the circle at angle ~90 degrees: true nearest point is there, ~1 mm away.
  const default_type probe_angle = 0.5 * pi;
  const vec3 probe(static_cast<default_type>((radius + 1.0) * std::cos(probe_angle)),
                   static_cast<default_type>((radius + 1.0) * std::sin(probe_angle)),
                   0.0);
  const default_type true_foot_s = s_max * (probe_angle / (2.0 * pi));

  // Adversarial warm start near the antipode (angle ~250 degrees), far from the true foot.
  const default_type bad_warm = s_max * ((250.0 / 360.0));
  const MR::DWI::Tractography::FootPoint::Foot foot = nearest_point(view, tension, probe, bad_warm, s_max);

  EXPECT_NEAR(foot.s, true_foot_s, 0.02 * s_max)
      << "foot converged away from the nearest point (antipodal overshoot regression)";
  EXPECT_NEAR(std::sqrt(foot.dist_sq), 1.0, 0.05);
}

// A CLOSED circular loop versus a uniform decimation of itself: a smoke test that the symmetric
//   Hausdorff of a full loop is small (bounded by the decimated chord-vs-arc sag), exercising the
//   seam / wrap-around path that open arcs do not.
TEST(HausdorffTest, ClosedLoopWithinBound) {
  const default_type radius = 40.0;
  const default_type step = 0.5;
  const Streamline<> dense = circular_arc(radius, step, 2.0 * pi);
  const size_t keep_every = 8;
  const Streamline<> coarse = decimate(dense, keep_every);
  const HausdorffResult r = hausdorff(dense, coarse);
  const default_type chord = static_cast<default_type>(keep_every) * step;
  const default_type sag_bound = (chord * chord) / (8.0 * radius);
  EXPECT_GT(r.distance, 0.0);
  EXPECT_LT(r.distance, sag_bound);
}

// Swapping the arguments must yield the same (symmetric) value.
TEST(HausdorffTest, Symmetry) {
  const Streamline<> dense = circular_arc(25.0, 0.5, 0.75 * pi);
  const Streamline<> coarse = decimate(dense, 6);
  const HausdorffResult ab = hausdorff(dense, coarse);
  const HausdorffResult ba = hausdorff(coarse, dense);
  EXPECT_NEAR(ab.distance, ba.distance, 1.0e-6);
}

// The ratio rule must yield a stable value as the input sampling step is varied: the reported
//   distance between an arc and its offset should not drift with how densely the inputs are sampled.
TEST(HausdorffTest, RatioRuleStableUnderStep) {
  const default_type radius = 20.0;
  const point_type delta(0.0f, 0.0f, 1.5f);
  default_type prev = -1.0;
  for (const default_type step : {1.5, 1.0, 0.5, 0.25}) {
    const Streamline<> a = circular_arc(radius, step, pi);
    const Streamline<> b = offset_by(a, delta);
    const HausdorffResult r = hausdorff(a, b);
    EXPECT_NEAR(r.distance, 1.5, 5.0e-3) << "step = " << step;
    if (prev >= 0.0)
      EXPECT_NEAR(r.distance, prev, 5.0e-3) << "step = " << step;
    prev = r.distance;
  }
}

// An optional threshold tightens probe spacing but must not change a well-resolved result.
TEST(HausdorffTest, ThresholdConfigConsistent) {
  const Streamline<> dense = circular_arc(30.0, 0.5, pi);
  const Streamline<> coarse = decimate(dense, 6);
  HausdorffConfig config;
  config.threshold_mm = 0.05;
  const HausdorffResult with_threshold = hausdorff(dense, coarse, config);
  const HausdorffResult without_threshold = hausdorff(dense, coarse);
  EXPECT_NEAR(with_threshold.distance, without_threshold.distance, 5.0e-3);
}

// Degenerate inputs must produce finite, well-defined results (never NaN).
TEST(HausdorffTest, DegenerateInputs) {
  const Streamline<> line = straight_line(10.0, 1.0);

  // Single-vertex "streamline": falls back to nearest-vertex distance from that point to the line.
  Streamline<> single;
  single.push_back(point_type(5.0f, 4.0f, 0.0f));
  const HausdorffResult r_single = hausdorff(line, single);
  EXPECT_TRUE(std::isfinite(r_single.distance));
  // The point sits 4 mm off the line at x=5 (an interior vertex), so the directed max is >= 4.
  EXPECT_GE(r_single.distance, 4.0 - 1.0e-6);

  // Two coincident streamlines that each collapse to a point.
  Streamline<> dot;
  dot.push_back(point_type(0.0f, 0.0f, 0.0f));
  dot.push_back(point_type(0.0f, 0.0f, 0.0f));
  const HausdorffResult r_dot = hausdorff(dot, dot);
  EXPECT_TRUE(std::isfinite(r_dot.distance));
  EXPECT_NEAR(r_dot.distance, 0.0, 1.0e-9);
}
