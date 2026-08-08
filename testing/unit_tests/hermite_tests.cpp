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

#include "math/hermite.h"
#include "types.h"

#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <vector>

namespace {

using value_type = MR::default_type;

// Scalar control points used to probe a single component of the spline.
constexpr std::array<value_type, 4> control_points{-0.7, 1.3, 2.1, -0.4};

// Evaluate the interpolated position component at parameter mu for a given tension.
value_type position_at(value_type tension, value_type mu) {
  MR::Math::Hermite<value_type> h(tension);
  h.set(mu);
  return h.value(control_points[0], control_points[1], control_points[2], control_points[3]);
}

// Evaluate the analytic parametric derivative dP/dmu at parameter mu for a given tension.
value_type derivative_at(value_type tension, value_type mu) {
  MR::Math::Hermite<value_type> h(tension);
  h.set(mu);
  return h.derivative(control_points[0], control_points[1], control_points[2], control_points[3]);
}

} // namespace

TEST(HermiteTest, DerivativeMatchesCentralDifference) {
  const std::vector<value_type> tensions{0.0, 0.1, 0.5, 1.0};
  const std::vector<value_type> mus{0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0};
  // Central-difference step; balances truncation (O(h^2)) and round-off for a smooth cubic.
  const value_type h = 1.0e-6;
  const value_type tolerance = 1.0e-7;

  for (const value_type tension : tensions) {
    for (const value_type mu : mus) {
      const value_type analytic = derivative_at(tension, mu);
      const value_type numeric = (position_at(tension, mu + h) - position_at(tension, mu - h)) / (2.0 * h);
      EXPECT_NEAR(analytic, numeric, tolerance)
          << std::setprecision(std::numeric_limits<value_type>::max_digits10) << "tension = " << tension
          << ", mu = " << mu << "\n  analytic = " << analytic << "\n  numeric  = " << numeric;
    }
  }
}

TEST(HermiteTest, ValueAndDerivativeConsistentWithSeparateAccessors) {
  const std::vector<value_type> tensions{0.0, 0.1, 0.5};
  const std::vector<value_type> mus{0.0, 0.3, 0.6, 1.0};
  const value_type tolerance = std::numeric_limits<value_type>::epsilon();

  for (const value_type tension : tensions) {
    for (const value_type mu : mus) {
      MR::Math::Hermite<value_type> hermite(tension);
      hermite.set(mu);
      const auto bundled =
          hermite.value_and_derivative(control_points[0], control_points[1], control_points[2], control_points[3]);
      EXPECT_NEAR(bundled.position,
                  hermite.value(control_points[0], control_points[1], control_points[2], control_points[3]),
                  tolerance);
      EXPECT_NEAR(bundled.derivative,
                  hermite.derivative(control_points[0], control_points[1], control_points[2], control_points[3]),
                  tolerance);
    }
  }
}

TEST(HermiteTest, ArrayAndPointerOverloadsAgree) {
  MR::Math::Hermite<value_type> hermite(0.1);
  hermite.set(0.42);
  const value_type scalar =
      hermite.derivative(control_points[0], control_points[1], control_points[2], control_points[3]);
  const value_type from_array = hermite.derivative(control_points);
  const value_type from_pointer = hermite.derivative(control_points.data());
  const value_type tolerance = std::numeric_limits<value_type>::epsilon();
  EXPECT_NEAR(scalar, from_array, tolerance);
  EXPECT_NEAR(scalar, from_pointer, tolerance);
}

// At a Catmull-Rom knot (mu = 1 on one segment == mu = 0 on the next), the reconstructed
// position must be C0-continuous (passing through the shared control point) and the parametric
// tangent must be C1-consistent across the join when sampled from the relevant 4-tuples.
TEST(HermiteTest, ContinuityAcrossSegmentJoin) {
  const value_type tension = 0.1;
  // Five collinear-but-irregular control points P0..P4 forming two adjacent segments:
  //   segment A interpolates between P1 and P2 from 4-tuple (P0, P1, P2, P3);
  //   segment B interpolates between P2 and P3 from 4-tuple (P1, P2, P3, P4).
  const std::array<value_type, 5> p{0.0, 1.0, 2.5, 4.0, 7.0};

  MR::Math::Hermite<value_type> hermite(tension);

  // End of segment A (mu = 1) should equal P2.
  hermite.set(1.0);
  const value_type pos_a_end = hermite.value(p[0], p[1], p[2], p[3]);
  // Start of segment B (mu = 0) should equal P2.
  hermite.set(0.0);
  const value_type pos_b_start = hermite.value(p[1], p[2], p[3], p[4]);

  const value_type c0_tolerance = 16.0 * std::numeric_limits<value_type>::epsilon();
  EXPECT_NEAR(pos_a_end, p[2], c0_tolerance);
  EXPECT_NEAR(pos_b_start, p[2], c0_tolerance);
  EXPECT_NEAR(pos_a_end, pos_b_start, c0_tolerance);

  // Catmull-Rom tangent at an interior knot Pi is t*(P_{i+1} - P_{i-1}) ... with this basis the
  // derivative at mu = 1 of segment A and mu = 0 of segment B both equal (0.5 - t)*(P_{i+1}-P_{i-1}).
  hermite.set(1.0);
  const value_type der_a_end = hermite.derivative(p[0], p[1], p[2], p[3]);
  hermite.set(0.0);
  const value_type der_b_start = hermite.derivative(p[1], p[2], p[3], p[4]);

  const value_type c1_tolerance = 1.0e-12;
  EXPECT_NEAR(der_a_end, der_b_start, c1_tolerance)
      << "tangent discontinuous across knot: " << der_a_end << " vs " << der_b_start;

  // Cross-check against the closed form (0.5 - t) * (P2 - P0) for the shared knot P1-side tangent.
  const value_type t = 0.5 * tension;
  const value_type expected_knot_tangent = (0.5 - t) * (p[3] - p[1]);
  EXPECT_NEAR(der_a_end, expected_knot_tangent, c1_tolerance);
}
