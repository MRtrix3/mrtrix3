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

#include "types.h"

#include "dwi/tractography/spline.h"

//! Foot-point (nearest-point) query of a probe against a tension-Catmull-Rom spline.
/*! Shared by the spline Hausdorff distance (stage 3.5) and the greedy knot-insertion slow
 *  decimator (stage 3.8): both need the same warm-started nearest-point solve against a
 *  reconstruction expressed through the reflected-ghost \c SplineView. The two were originally
 *  duplicated; this header is the single source of truth.
 *
 *  Position and derivative are evaluated in \c default_type (double) precision so the Newton
 *  solve is well-conditioned regardless of the streamline's storage precision. */
namespace MR::DWI::Tractography::FootPoint {

//! Position / derivative working type for the foot-point solve.
using vec3 = Eigen::Matrix<default_type, 3, 1>;

//! Newton-Raphson controls for the foot-point solve.
constexpr size_t newton_max_iterations = 16;
//! Convergence on the global parameter s (dimensionless); ~1e-5 of a segment is sub-micron at any
//!   realistic vertex spacing.
constexpr default_type newton_tolerance = 1.0e-5;
//! Below this squared parametric speed the foot-point condition has no usable gradient and the
//!   Newton step is abandoned in favour of the bracketed fallback.
constexpr default_type min_speed_squared = 1.0e-12;
//! Number of subdivisions of the warm-start bracket used by the fallback local search when Newton
//!   fails to converge (e.g. at a parametric speed null or a sharp foot ambiguity).
constexpr size_t fallback_search_subdivisions = 32;
//! Width of the bracketed-fallback window, as a fraction of the spline's parametric span, used
//!   when Newton fails to converge from the warm start.
constexpr default_type fallback_window_fraction = 0.25;

//! State of a converged (or fallen-back) foot-point search on a target spline.
struct Foot {
  default_type s;       //!< Global parameter of the nearest point on the target spline.
  default_type dist_sq; //!< Squared distance (mm^2) from the probe to that nearest point.
};

//! Squared Euclidean distance between two points (mm^2), in double precision.
inline default_type distance_squared(const vec3 &p, const vec3 &q) { return (p - q).squaredNorm(); }

//! Evaluate the squared probe-to-spline distance at a candidate global parameter.
template <typename ValueType>
inline default_type foot_distance_squared(const SplineView<ValueType> &target,
                                          const default_type tension,
                                          const vec3 &probe,
                                          const default_type s) {
  const typename SplineView<ValueType>::point_type p = target.position(s, static_cast<ValueType>(tension));
  return distance_squared(probe, p.template cast<default_type>());
}

//! Bracketed local search fallback: sample the warm-start bracket and return its best parameter.
/*! Invoked only when Newton-Raphson fails to converge (parametric-speed null, sharp foot ambiguity).
 *  Searches \c [lo, hi] on a uniform grid then refines around the best sample by golden-section, so
 *  a bad foot is never silently accepted. */
template <typename ValueType>
inline Foot bracketed_search(const SplineView<ValueType> &target,
                             const default_type tension,
                             const vec3 &probe,
                             const default_type lo,
                             const default_type hi) {
  default_type best_s = lo;
  default_type best_d2 = foot_distance_squared(target, tension, probe, lo);
  for (size_t i = 1; i <= fallback_search_subdivisions; ++i) {
    const default_type s =
        lo + (hi - lo) * static_cast<default_type>(i) / static_cast<default_type>(fallback_search_subdivisions);
    const default_type d2 = foot_distance_squared(target, tension, probe, s);
    if (d2 < best_d2) {
      best_d2 = d2;
      best_s = s;
    }
  }
  // Golden-section refinement within the cell surrounding the best grid sample.
  const default_type cell = (hi - lo) / static_cast<default_type>(fallback_search_subdivisions);
  default_type a = std::max(lo, best_s - cell);
  default_type b = std::min(hi, best_s + cell);
  constexpr default_type inv_phi = 0.6180339887498949; // 1/golden-ratio
  default_type c = b - inv_phi * (b - a);
  default_type d = a + inv_phi * (b - a);
  default_type fc = foot_distance_squared(target, tension, probe, c);
  default_type fd = foot_distance_squared(target, tension, probe, d);
  for (size_t i = 0; i != newton_max_iterations; ++i) {
    if (fc < fd) {
      b = d;
      d = c;
      fd = fc;
      c = b - inv_phi * (b - a);
      fc = foot_distance_squared(target, tension, probe, c);
    } else {
      a = c;
      c = d;
      fc = fd;
      d = a + inv_phi * (b - a);
      fd = foot_distance_squared(target, tension, probe, d);
    }
  }
  const default_type s = 0.5 * (a + b);
  return {s, foot_distance_squared(target, tension, probe, s)};
}

//! Nearest point on the target spline to \c probe, by warm-started Gauss-Newton on the foot-point
//!   condition, with a bracketed fallback on non-convergence.
/*! Solves \f$f(s)=(Q-P(s))\cdot P'(s)=0\f$. The exact Jacobian is
 *  \f$f'(s) = -\lVert P'(s)\rVert^2 + (Q-P(s))\cdot P''(s)\f$; we drop the curvature term and use the
 *  Gauss-Newton approximation \f$f'(s)\approx -\lVert P'(s)\rVert^2\f$. This avoids needing an
 *  analytic Hermite second derivative (not provided by stage 3.1), is unconditionally a descent
 *  direction on the squared distance near the foot, and converges quadratically there because the
 *  dropped term vanishes as the residual \f$(Q-P)\f$ aligns with the normal; it is robust and
 *  sufficient for the proximal, co-oriented regime assumed here.
 *
 *  \c s_max is \c segments = size()-1. The endpoints are handled by clamping: a probe whose true
 *  nearest point lies past an endpoint converges to (and is pinned at) that endpoint. */
template <typename ValueType>
inline Foot nearest_point(const SplineView<ValueType> &target,
                          const default_type tension,
                          const vec3 &probe,
                          const default_type warm_start,
                          const default_type s_max) {
  const ValueType tension_v = static_cast<ValueType>(tension);
  default_type s = std::min(std::max<default_type>(0.0, warm_start), s_max);
  bool converged = false;
  for (size_t iter = 0; iter != newton_max_iterations; ++iter) {
    const typename SplineView<ValueType>::point_type position = target.position(s, tension_v);
    const typename SplineView<ValueType>::point_type tangent = target.tangent(s, tension_v);
    const vec3 residual = probe - position.template cast<default_type>();
    const vec3 deriv = tangent.template cast<default_type>();
    const default_type speed_sq = deriv.squaredNorm();
    if (speed_sq < min_speed_squared)
      break; // Parametric-speed null: hand over to the bracketed fallback.
    const default_type f = residual.dot(deriv);
    // Gauss-Newton: f'(s) ~= -||P'(s)||^2 (drop the (Q-P).P'' curvature term).
    const default_type step = f / speed_sq; // = -f / f'(s)
    default_type s_next = s + step;
    s_next = std::min(std::max<default_type>(0.0, s_next), s_max);
    const default_type delta = std::fabs(s_next - s);
    s = s_next;
    if (delta < newton_tolerance) {
      converged = true;
      break;
    }
  }
  if (converged) {
    const default_type d2 = foot_distance_squared(target, tension, probe, s);
    // Endpoints: the clamp above already pins a probe whose foot lies past an end to that end.
    return {s, d2};
  }
  // Non-convergence: search a local bracket around the warm start rather than accept a bad foot.
  const default_type half_window = std::max<default_type>(1.0, fallback_window_fraction * s_max);
  const default_type lo = std::max<default_type>(0.0, warm_start - half_window);
  const default_type hi = std::min<default_type>(s_max, warm_start + half_window);
  return bracketed_search(target, tension, probe, lo, hi);
}

} // namespace MR::DWI::Tractography::FootPoint
