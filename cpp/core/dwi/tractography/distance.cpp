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

#include "dwi/tractography/distance.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/foot_point.h"
#include "dwi/tractography/spline.h"

namespace MR::DWI::Tractography {

namespace {

using point_type = Streamline<>::point_type;
//! Position / derivative are evaluated in double precision for a well-conditioned Newton solve.
using FootPoint::vec3;
//! The warm-started foot-point machinery (Foot, nearest_point, bracketed search) is shared with the
//!   stage-3.8 slow decimator; it lives in dwi/tractography/foot_point.h.
using FootPoint::Foot;

// ---------------------------------------------------------------------------------------------
// Named constants (no magic numbers). All dimensionless unless stated.
// ---------------------------------------------------------------------------------------------

//! Two vertices closer than this (mm) are treated as coincident (a zero-length step).
constexpr default_type coincident_step_mm = 1.0e-6;

//! Probe-spacing rule, governing fractions (see choose_upsample_ratio() for the derivation).
//! Fraction of the discretisation budget retained: the chord-vs-arc sampling error is held to
//!   \c discretisation_fraction times the quantity of interest (threshold, or vertex scale).
constexpr default_type discretisation_fraction = 0.1;
//! When no Hausdorff threshold is supplied, the effective threshold is this fraction of the mean
//!   inter-vertex spacing, so the sampling error sits well below the vertex scale.
constexpr default_type threshold_fraction_of_spacing = 0.25;
//! Upper bound on probe spacing as a fraction of the minimum radius of curvature, capping the
//!   per-step turn angle to keep the warm-started Newton-Raphson well-conditioned through tight
//!   bends (a probe spacing of \c newton_conditioning_fraction * R_min subtends that many radians).
constexpr default_type newton_conditioning_fraction = 0.25;
//! High percentile of the per-vertex curvature taken as a robust maximum, so a single noisy vertex
//!   does not collapse R_min (and thus blow the probe count up). 0.95 -> the 95th percentile.
constexpr default_type curvature_robust_percentile = 0.95;
//! Hard ceiling on the upsample ratio, guarding against pathological (near-zero) R_min estimates.
constexpr size_t max_upsample_ratio = 64;

// ---------------------------------------------------------------------------------------------

//! Mean inter-vertex spacing (mm) of a streamline; 0 for fewer than two vertices.
default_type mean_spacing(const Streamline<> &tck) {
  if (tck.size() < 2)
    return 0.0;
  default_type total = 0.0;
  for (size_t i = 1; i != tck.size(); ++i)
    total += (tck[i].template cast<default_type>() - tck[i - 1].template cast<default_type>()).norm();
  return total / static_cast<default_type>(tck.size() - 1);
}

//! Robust minimum radius of curvature (mm) over a streamline: the reciprocal of a high-percentile
//!   curvature. Returns no value when curvature is everywhere (near) zero (a straight curve), in
//!   which case the radius is effectively infinite and only the threshold/vertex scale governs.
std::optional<default_type> robust_min_radius(const Streamline<> &tck, const CurvatureConfig &curvature_config) {
  std::vector<default_type> kappa = curvature(tck, curvature_config);
  if (kappa.empty())
    return std::nullopt;
  std::sort(kappa.begin(), kappa.end());
  const size_t idx = std::min(
      kappa.size() - 1, static_cast<size_t>(curvature_robust_percentile * static_cast<default_type>(kappa.size() - 1)));
  const default_type kappa_robust = kappa[idx];
  if (kappa_robust <= 0.0)
    return std::nullopt;
  return 1.0 / kappa_robust;
}

//! Determine the integer ratio by which to upsample a streamline's segments before probing.
/*! \par Derivation
 *  Sampling a curve of local radius \f$R\f$ at probe spacing \f$\delta s\f$ approximates each arc by
 *  its chord; the chord sags from the arc by \f$\approx \delta s^2 / (8 R)\f$. To hold this
 *  discretisation error to a fraction \f$\eta\f$ of the quantity of interest \f$\tau_{eff}\f$ we
 *  require \f$\delta s^2 / (8 R_{min}) \le \eta\,\tau_{eff}\f$, i.e.
 *  \f$\delta s \le \sqrt{8\,R_{min}\,\eta\,\tau_{eff}}\f$. \f$\tau_{eff}\f$ is the caller's Hausdorff
 *  threshold when supplied, else a small fraction of the inter-vertex spacing \f$\Delta\f$. A second
 *  cap \f$\delta s \le f\,R_{min}\f$ keeps the per-step turn angle small for Newton conditioning.
 *  The ratio is \f$\max(1, \lceil \Delta / \delta s \rceil)\f$.
 *
 *  \param spacing       mean inter-vertex spacing \f$\Delta\f$ (mm) of the streamline being probed.
 *  \param min_radius    robust minimum radius of curvature \f$R_{min}\f$ (mm) over both streamlines;
 *                       absent when both are (locally) straight, in which case only the threshold
 *                       governs and the curvature caps are dropped. */
size_t choose_upsample_ratio(const default_type spacing,
                             const std::optional<default_type> &min_radius,
                             const std::optional<default_type> &threshold_mm) {
  if (spacing <= 0.0)
    return 1;
  const default_type tau_eff = threshold_mm.has_value() ? *threshold_mm : threshold_fraction_of_spacing * spacing;
  // Default to the full inter-vertex span (ratio 1) and only tighten if a finite radius applies.
  default_type delta_s = spacing;
  if (min_radius.has_value() && *min_radius > 0.0) {
    const default_type from_sag =
        std::sqrt(8.0 * (*min_radius) * discretisation_fraction * std::max<default_type>(tau_eff, 0.0));
    const default_type from_conditioning = newton_conditioning_fraction * (*min_radius);
    delta_s = std::min({spacing, from_sag, from_conditioning});
  } else if (tau_eff > 0.0) {
    // Straight (or near-straight) geometry: the chord sag vanishes, so the threshold alone does not
    //   bound delta_s; a single segment reproduces the curve exactly. Keep ratio 1.
    delta_s = spacing;
  }
  if (!(delta_s > 0.0))
    return 1;
  const default_type ratio = std::ceil(spacing / delta_s);
  if (!std::isfinite(ratio) || ratio < 1.0)
    return 1;
  return std::min(static_cast<size_t>(ratio), max_upsample_ratio);
}

//! Cumulative chord length (mm) at each vertex: \c cum[0]=0, \c cum[j]=cum[j-1]+|v_j - v_{j-1}|.
/*! The piecewise-linear (chord) approximation is used only to SEED the foot-point search; the Newton
 *  solve that follows refines to the true spline foot, so the sub-segment arc-vs-chord error of the
 *  seed is immaterial. */
std::vector<default_type> cumulative_chord_length(const Streamline<> &tck) {
  std::vector<default_type> cum(tck.size(), 0.0);
  for (size_t j = 1; j != tck.size(); ++j)
    cum[j] = cum[j - 1] + (tck[j].template cast<default_type>() - tck[j - 1].template cast<default_type>()).norm();
  return cum;
}

//! Cumulative chord length at a global spline parameter \c s in [0, n-1] (linear within the chord).
default_type param_to_length(const std::vector<default_type> &cum, const default_type s) {
  const default_type s_max = static_cast<default_type>(cum.size() - 1);
  const default_type clamped = std::min(std::max<default_type>(0.0, s), s_max);
  const size_t seg = std::min(static_cast<size_t>(clamped), cum.size() - 2);
  const default_type mu = clamped - static_cast<default_type>(seg);
  return cum[seg] + mu * (cum[seg + 1] - cum[seg]);
}

//! Inverse of \c param_to_length: the global spline parameter whose cumulative chord length is \c len.
/*! Binary search over the (monotone) cumulative table, returning \c segment+mu so the result indexes
 *  the same reflected-ghost spline the foot-point solve runs against. */
default_type length_to_param(const std::vector<default_type> &cum, const default_type len) {
  const default_type total = cum.back();
  if (!(total > 0.0))
    return 0.0;
  const default_type clamped = std::min(std::max<default_type>(0.0, len), total);
  // First vertex whose cumulative length exceeds the query; the foot lies in the preceding chord.
  const auto it = std::upper_bound(cum.begin(), cum.end(), clamped);
  size_t hi = static_cast<size_t>(it - cum.begin());
  if (hi == 0)
    return 0.0;
  if (hi >= cum.size())
    hi = cum.size() - 1;
  const size_t lo = hi - 1;
  const default_type chord = cum[hi] - cum[lo];
  const default_type mu = chord > 0.0 ? (clamped - cum[lo]) / chord : 0.0;
  return static_cast<default_type>(lo) + mu;
}

//! Directed Hausdorff distance d(source -> target): max over probes on the source spline of the
//!   nearest distance to the target spline. Each foot is seeded by arc-length correspondence.
struct DirectedResult {
  default_type distance;         //!< Directed Hausdorff distance (mm).
  default_type argmax_parameter; //!< Source global parameter attaining the maximum.
};

DirectedResult directed_hausdorff(const Streamline<> &source,
                                  const Streamline<> &target,
                                  const default_type tension,
                                  const std::optional<default_type> &min_radius,
                                  const std::optional<default_type> &threshold_mm) {
  const SplineView<> source_view(source);
  const SplineView<> target_view(target);
  const size_t source_segments = source.size() - 1;
  const default_type target_s_max = static_cast<default_type>(target.size() - 1);

  const default_type spacing = mean_spacing(source);
  const size_t ratio = choose_upsample_ratio(spacing, min_radius, threshold_mm);

  // Probes are sampled at global parameter s = segment + i/ratio across the whole source spline, and
  //   each foot on the target is sought by an independent ARC-LENGTH-CORRESPONDENCE seed: the probe
  //   at fractional arc length f along the source is searched from the target parameter at the same
  //   fractional arc length. Under the proximity + co-orientation assumption this seed lies near the
  //   true foot for every probe on its own, so -- unlike a previous-foot ("marching") warm start,
  //   which can stick or jump to the wrong branch on a CLOSED LOOP or self-approaching curve and,
  //   being monotone, never recover (yielding a nearest distance on the order of the loop diameter) --
  //   the search cannot accumulate drift. The cost stays sub-quadratic: O(n+m) to build the chord
  //   tables once, then O(log m) per probe to invert the target arc length.
  const std::vector<default_type> source_length = cumulative_chord_length(source);
  const std::vector<default_type> target_length = cumulative_chord_length(target);
  const default_type source_total = source_length.back();
  const default_type target_total = target_length.back();

  // *** Assumption boundary ***
  // Arc-length correspondence still presumes proximity + matching orientation. For general
  // (crossing / antiparallel / non-proximal) pairs the seed may land on the wrong branch and the
  // per-probe seed must be replaced by a global nearest-point or Frechet-style search.
  default_type max_dist_sq = -1.0;
  default_type argmax_parameter = 0.0;
  const size_t total_probes = source_segments * ratio + 1;
  for (size_t p = 0; p != total_probes; ++p) {
    const default_type s_source = std::min(static_cast<default_type>(p) / static_cast<default_type>(ratio),
                                           static_cast<default_type>(source_segments));
    const point_type q = source_view.position(s_source, static_cast<SplineView<>::value_type>(tension));
    const vec3 probe = q.template cast<default_type>();
    default_type seed = 0.0;
    if (source_total > 0.0 && target_total > 0.0) {
      const default_type fraction = param_to_length(source_length, s_source) / source_total;
      seed = length_to_param(target_length, fraction * target_total);
    }
    const Foot foot = FootPoint::nearest_point(target_view, tension, probe, seed, target_s_max);
    if (foot.dist_sq > max_dist_sq) {
      max_dist_sq = foot.dist_sq;
      argmax_parameter = s_source;
    }
  }
  return {std::sqrt(std::max<default_type>(0.0, max_dist_sq)), argmax_parameter};
}

//! Distance (mm) from a single point to the nearest endpoint of a streamline (degenerate fallback).
default_type point_to_streamline_endpoints(const vec3 &p, const Streamline<> &tck) {
  default_type best = std::numeric_limits<default_type>::infinity();
  for (const auto &v : tck)
    best = std::min(best, (p - v.template cast<default_type>()).norm());
  return best;
}

//! Symmetric distance for the degenerate case where at least one streamline lacks a spline (size<2
//!   or collapsed to a single distinct vertex): fall back to nearest-vertex distances both ways.
HausdorffResult degenerate_hausdorff(const Streamline<> &a, const Streamline<> &b) {
  // Treat empty streamlines as having no geometry: distance is zero (nothing to compare).
  if (a.empty() && b.empty())
    return {0.0, 0.0, true};
  if (a.empty() || b.empty())
    return {std::numeric_limits<default_type>::infinity(), 0.0, true};
  default_type max_a_to_b = 0.0;
  default_type argmax_a = 0.0;
  for (size_t i = 0; i != a.size(); ++i) {
    const default_type d = point_to_streamline_endpoints(a[i].template cast<default_type>(), b);
    if (d > max_a_to_b) {
      max_a_to_b = d;
      argmax_a = static_cast<default_type>(i);
    }
  }
  default_type max_b_to_a = 0.0;
  default_type argmax_b = 0.0;
  for (size_t i = 0; i != b.size(); ++i) {
    const default_type d = point_to_streamline_endpoints(b[i].template cast<default_type>(), a);
    if (d > max_b_to_a) {
      max_b_to_a = d;
      argmax_b = static_cast<default_type>(i);
    }
  }
  if (max_a_to_b >= max_b_to_a)
    return {max_a_to_b, argmax_a, true};
  return {max_b_to_a, argmax_b, false};
}

//! Whether a streamline supports a spline: at least two vertices and at least two distinct ones.
bool has_spline(const Streamline<> &tck) {
  if (tck.size() < 2)
    return false;
  const vec3 first = tck.front().template cast<default_type>();
  for (size_t i = 1; i != tck.size(); ++i) {
    if ((tck[i].template cast<default_type>() - first).norm() > coincident_step_mm)
      return true;
  }
  return false;
}

} // namespace

HausdorffResult hausdorff(const Streamline<> &a, const Streamline<> &b, const HausdorffConfig &config) {
  // Degenerate guard: if either streamline cannot be splined, fall back to vertex distances; the
  // result is always finite (or +inf for an empty-vs-nonempty pair), never NaN.
  if (!has_spline(a) || !has_spline(b))
    return degenerate_hausdorff(a, b);

  // R_min taken over both streamlines (the more conservative bound) so the probe spacing is adequate
  // for either curve's tightest bend and Newton stays well-conditioned on either target.
  std::optional<default_type> min_radius;
  {
    const std::optional<default_type> ra = robust_min_radius(a, config.curvature);
    const std::optional<default_type> rb = robust_min_radius(b, config.curvature);
    if (ra.has_value() && rb.has_value())
      min_radius = std::min(*ra, *rb);
    else if (ra.has_value())
      min_radius = ra;
    else
      min_radius = rb;
  }

  const DirectedResult a_to_b = directed_hausdorff(a, b, config.tension, min_radius, config.threshold_mm);
  const DirectedResult b_to_a = directed_hausdorff(b, a, config.tension, min_radius, config.threshold_mm);

  if (a_to_b.distance >= b_to_a.distance)
    return {a_to_b.distance, a_to_b.argmax_parameter, true};
  return {b_to_a.distance, b_to_a.argmax_parameter, false};
}

} // namespace MR::DWI::Tractography
