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

#include "dwi/tractography/curvature.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "exception.h"
#include "mrtrix.h"

#include "dwi/tractography/properties.h"

namespace MR::DWI::Tractography {

namespace {

using point_type = Streamline<>::point_type;

//! Two coincident vertices closer than this (mm) are treated as a zero-length step and collapsed.
constexpr default_type coincident_step_mm = 1.0e-6;
//! Below this first-derivative magnitude the local speed underflows and curvature is reported as 0.
constexpr default_type min_speed = 1.0e-6;

//! AUTO scale tuning constants (dimensionless / geometry-relative; see curvature.md section 5).
//! Minimum smoothing scale (a multiple of the mean step); also the scale for clean, well-correlated
//!   geometry, where a tight window preserves genuine high-curvature bends.
constexpr default_type min_scale_steps = 2.0;
//! Smoothing scale (a multiple of the mean step) applied when the turn angles are fully decorrelated
//!   (probabilistic wiggle); a wide window is required to average the noise band away.
constexpr default_type noise_scale_steps = 12.0;
//! Number of distinct vertices below which AUTO drops straight to the FIXED fallback.
constexpr size_t min_vertices_for_auto = 5;
//! Turn-angle lag-2 autocorrelation at/above which the geometry is treated as fully correlated
//!   (smooth anatomy); at/below 0 it is treated as fully decorrelated (pure wiggle); the scale is
//!   interpolated in between. Lag 2 rather than lag 1 is used because a single jittered vertex is
//!   shared by two consecutive step directions, inducing a spurious positive lag-1 correlation even
//!   in pure wiggle; that artefact does not reach lag 2, so lag 2 cleanly separates the two regimes.
constexpr default_type full_correlation_threshold = std::exp(-1.0);

//! Distinct-vertex subsequence: collapsed geometry plus a map back to the full vertex indexing.
struct DistinctVertices {
  std::vector<point_type> positions;        //!< One representative per run of coincident vertices.
  std::vector<default_type> arclength;      //!< Cumulative arc length (mm) of the representatives.
  std::vector<size_t> original_to_distinct; //!< For every original vertex, its representative index.
};

//! Collapse runs of coincident vertices and accumulate cumulative arc length in one O(N) pass.
DistinctVertices collapse_and_accumulate(const Streamline<> &tck) {
  DistinctVertices out;
  out.positions.reserve(tck.size());
  out.arclength.reserve(tck.size());
  out.original_to_distinct.resize(tck.size());

  out.positions.push_back(tck[0]);
  out.arclength.push_back(0.0);
  out.original_to_distinct[0] = 0;
  for (size_t i = 1; i != tck.size(); ++i) {
    const default_type step = (tck[i] - out.positions.back()).norm();
    if (step < coincident_step_mm) {
      // Coincident with the current representative: reuse it, contribute no arc length.
      out.original_to_distinct[i] = out.positions.size() - 1;
      continue;
    }
    out.arclength.push_back(out.arclength.back() + step);
    out.positions.push_back(tck[i]);
    out.original_to_distinct[i] = out.positions.size() - 1;
  }
  return out;
}

//! Per-step unit turn angles (radians) between consecutive segment directions of the representatives.
std::vector<default_type> turn_angles(const std::vector<point_type> &positions) {
  std::vector<point_type> directions;
  directions.reserve(positions.size() - 1);
  for (size_t i = 1; i != positions.size(); ++i)
    directions.push_back((positions[i] - positions[i - 1]).normalized());
  std::vector<default_type> angles;
  if (directions.size() < 2)
    return angles;
  angles.reserve(directions.size() - 1);
  for (size_t i = 1; i != directions.size(); ++i) {
    const default_type dot =
        std::max<default_type>(-1.0, std::min<default_type>(1.0, directions[i].dot(directions[i - 1])));
    angles.push_back(std::acos(dot));
  }
  return angles;
}

//! Lag-2 autocorrelation of the per-step turn angles; std::nullopt if it cannot be estimated.
/*! Genuine anatomical curvature produces turn angles that remain correlated over several steps (a
 *  high lag-2 autocorrelation), whereas probabilistic wiggle is decorrelated beyond the shared-vertex
 *  artefact (lag-2 autocorrelation near zero or negative). This single statistic discriminates smooth
 *  geometry from wiggle and so drives how aggressively the curvature must be smoothed. Returns
 *  std::nullopt when the angles are constant (no variance) or too few to estimate. */
std::optional<default_type> turn_angle_lag2_autocorrelation(const std::vector<default_type> &angles) {
  if (angles.size() < 4)
    return std::nullopt;
  default_type mean = 0.0;
  for (const default_type a : angles)
    mean += a;
  mean /= static_cast<default_type>(angles.size());
  default_type variance = 0.0;
  for (const default_type a : angles)
    variance += (a - mean) * (a - mean);
  if (variance < min_speed)
    return std::nullopt; // Constant turn angle: no wiggle to detect, treat as fully correlated.

  default_type covariance = 0.0;
  for (size_t i = 2; i != angles.size(); ++i)
    covariance += (angles[i] - mean) * (angles[i - 2] - mean);
  return covariance / variance;
}

//! One representative position per parent arc, decimating sub-step runs of \c grouping vertices.
/*! When contiguous vertices are sub-step samples of a single deterministically-smooth parent arc
 *  (e.g. un-downsampled iFOD2 integration steps), the autocorrelation discriminant must be evaluated
 *  on one vertex per arc so the within-arc structure is excluded. The stride is the grouping rounded
 *  to the nearest integer (at least 1). */
std::vector<point_type> decimate_to_parent_arcs(const std::vector<point_type> &positions, const default_type grouping) {
  const size_t stride = std::max<size_t>(1, static_cast<size_t>(std::lround(grouping)));
  std::vector<point_type> out;
  out.reserve(positions.size() / stride + 1);
  for (size_t i = 0; i < positions.size(); i += stride)
    out.push_back(positions[i]);
  return out;
}

//! Resolve the arc-length smoothing scale (mm) for one streamline per curvature.md section 5.
/*! The scale is anchored to the streamline's own mean step and interpolated, by the turn-angle
 *  lag-2 autocorrelation, between a tight window for smooth geometry (which preserves genuine
 *  high-curvature bends) and a wide window for decorrelated wiggle (which averages the noise away),
 *  then bracketed into [2*step, L/4]. No absolute mm constant is involved.
 *
 *  When \c config.vertices_per_parent_arc exceeds 1, contiguous vertices are sub-step samples of a
 *  single parent arc rather than independent geometric samples; the mean step is scaled up by that
 *  factor (so the scale ladder is measured in independent-sample widths, not sub-steps) and the
 *  autocorrelation is estimated on one vertex per arc (so the deterministic within-arc structure is
 *  not read as anatomical smoothness). With the default factor of 1 both adjustments are no-ops. */
default_type resolve_scale(const DistinctVertices &distinct, const CurvatureConfig &config) {
  const size_t n = distinct.positions.size();
  const default_type total_length = distinct.arclength.back();
  const default_type mean_step = total_length / static_cast<default_type>(n - 1);
  const default_type grouping = std::max<default_type>(1.0, config.vertices_per_parent_arc);
  // Step between independent geometric samples (one parent arc), in mm.
  const default_type effective_step = grouping * mean_step;
  const default_type scale_min = min_scale_steps * effective_step;
  const default_type scale_max = std::max(total_length / 4.0, scale_min);

  if (config.scale == CurvatureScale::FIXED)
    return std::max(config.fixed_scale_mm, scale_min);

  if (n < min_vertices_for_auto)
    return std::max(config.fixed_scale_mm, scale_min);

  const std::vector<default_type> angles = (grouping > 1.0)
                                               ? turn_angles(decimate_to_parent_arcs(distinct.positions, grouping))
                                               : turn_angles(distinct.positions);
  const std::optional<default_type> rho2 = turn_angle_lag2_autocorrelation(angles);

  // Map the lag-2 autocorrelation to a 0..1 "wiggle fraction": fully correlated geometry (rho2 at
  //   or above the 1/e threshold, or constant angles) wants the tight scale; fully decorrelated
  //   wiggle (rho2 <= 0) wants the wide noise-suppression scale; intermediate values interpolate.
  default_type wiggle_fraction = 0.0;
  if (rho2.has_value() && rho2.value() < full_correlation_threshold) {
    const default_type clamped = std::max<default_type>(0.0, rho2.value());
    wiggle_fraction = (full_correlation_threshold - clamped) / full_correlation_threshold;
  }
  const default_type scale_steps = min_scale_steps + wiggle_fraction * (noise_scale_steps - min_scale_steps);
  const default_type scale = scale_steps * effective_step;

  return std::max(scale_min, std::min(scale, scale_max));
}

//! Weighting of the local fit, derived from the requested curvature method.
enum class WindowWeighting { UNIFORM, GAUSSIAN };

//! Weight for a window sample at centred arc-length offset \c x given half-extent / sigma \c scale.
default_type sample_weight(const WindowWeighting weighting, const default_type x, const default_type scale) {
  switch (weighting) {
  case WindowWeighting::UNIFORM:
    return 1.0;
  case WindowWeighting::GAUSSIAN: {
    const default_type z = x / scale;
    return std::exp(-0.5 * z * z);
  }
  }
  return 1.0;
}

//! First and second arc-length derivatives of the geometry at one representative vertex.
struct Derivatives {
  point_type first;
  point_type second;
};

//! Weighted local-polynomial fit about representative \c centre; reads r', r'' at the centre.
/*! Builds one weighted normal-equation system (the shared Vandermonde design over the window's
 *  centred arc-length offsets) and solves it against the three spatial coordinates simultaneously.
 *  For a degree-d polynomial r_c(x) = a0 + a1 x + a2 x^2 + ..., the centre derivatives are
 *  r'_c = a1 and r''_c = 2 a2. */
Derivatives fit_derivatives(const DistinctVertices &distinct,
                            const size_t centre,
                            const default_type half_window,
                            const size_t order,
                            const WindowWeighting weighting,
                            const default_type weight_scale) {
  const size_t n = distinct.positions.size();
  const default_type s_centre = distinct.arclength[centre];

  // Grow the window symmetrically until at least (2*order + 1) samples are enclosed, so the
  //   least-squares system is determined even where the nominal half-window is too narrow.
  const size_t min_samples = 2 * order + 1;
  size_t lo = centre;
  size_t hi = centre;
  default_type effective_half = half_window;
  while (true) {
    lo = centre;
    while (lo > 0 && (s_centre - distinct.arclength[lo - 1]) <= effective_half)
      --lo;
    hi = centre;
    while (hi + 1 < n && (distinct.arclength[hi + 1] - s_centre) <= effective_half)
      ++hi;
    if ((hi - lo + 1) >= min_samples || (lo == 0 && hi == n - 1))
      break;
    effective_half *= 1.5;
  }

  const size_t ncoef = order + 1;
  Eigen::MatrixXd normal = Eigen::MatrixXd::Zero(ncoef, ncoef);
  Eigen::MatrixXd rhs = Eigen::MatrixXd::Zero(ncoef, 3);

  for (size_t j = lo; j <= hi; ++j) {
    const default_type x = distinct.arclength[j] - s_centre;
    const default_type w = sample_weight(weighting, x, weight_scale);
    // Powers of x up to the polynomial order, weighted into the normal equations.
    Eigen::VectorXd powers(ncoef);
    default_type p = 1.0;
    for (size_t k = 0; k != ncoef; ++k) {
      powers[k] = p;
      p *= x;
    }
    normal.noalias() += w * (powers * powers.transpose());
    for (size_t c = 0; c != 3; ++c)
      rhs.col(c) += w * powers * static_cast<default_type>(distinct.positions[j][c]);
  }

  const Eigen::MatrixXd coefficients = normal.colPivHouseholderQr().solve(rhs);
  Derivatives derivs;
  for (size_t c = 0; c != 3; ++c) {
    derivs.first[c] = coefficients(1, c);
    derivs.second[c] = 2.0 * coefficients(2, c);
  }
  return derivs;
}

//! Curvature magnitude (1/mm) from arc-length derivatives via kappa = ||r' x r''|| / ||r'||^3.
default_type curvature_from_derivatives(const Derivatives &derivs) {
  const default_type speed = derivs.first.norm();
  if (speed < min_speed)
    return 0.0;
  const default_type numerator = derivs.first.cross(derivs.second).norm();
  return numerator / (speed * speed * speed);
}

//! Shared core: distinct-vertex collapse, scale resolution, windowed fit, write-back to full length.
std::vector<default_type> curvature_impl(const Streamline<> &tck, const CurvatureConfig &config) {
  if (config.polynomial_order < 2)
    throw Exception("Curvature estimation requires a polynomial order of at least 2");

  if (tck.size() < 3)
    return std::vector<default_type>(tck.size(), 0.0);

  const DistinctVertices distinct = collapse_and_accumulate(tck);
  if (distinct.positions.size() < 3)
    return std::vector<default_type>(tck.size(), 0.0);

  const default_type scale = resolve_scale(distinct, config);
  const WindowWeighting weighting =
      (config.method == CurvatureMethod::GAUSSIAN_DERIV) ? WindowWeighting::GAUSSIAN : WindowWeighting::UNIFORM;
  // For a Gaussian weighting the scale is sigma and the truncation half-window is 3 sigma;
  //   for a uniform Savitzky-Golay weighting the scale is itself the half-window.
  const default_type half_window = (weighting == WindowWeighting::GAUSSIAN) ? (3.0 * scale) : scale;

  std::vector<default_type> distinct_curvature(distinct.positions.size(), 0.0);
  for (size_t i = 0; i != distinct.positions.size(); ++i) {
    const Derivatives derivs = fit_derivatives(distinct, i, half_window, config.polynomial_order, weighting, scale);
    distinct_curvature[i] = curvature_from_derivatives(derivs);
  }

  std::vector<default_type> result(tck.size());
  for (size_t i = 0; i != tck.size(); ++i)
    result[i] = distinct_curvature[distinct.original_to_distinct[i]];
  return result;
}

} // namespace

std::vector<default_type> curvature(const Streamline<> &tck, const CurvatureConfig &config) {
  return curvature_impl(tck, config);
}

std::vector<default_type> curvature(const SplineView<> &view, const CurvatureConfig &config) {
  // The smoothing operates on the raw vertices and their cumulative arc length, so a lightweight
  //   copy of the view's underlying vertices is sufficient; no Hermite reconstruction is involved.
  Streamline<> tck;
  tck.reserve(view.size());
  for (size_t i = 0; i != view.size(); ++i)
    tck.push_back(view[static_cast<ssize_t>(i)]);
  return curvature_impl(tck, config);
}

void configure_from_properties(CurvatureConfig &config, const Properties &properties) {
  const auto method = properties.find("method");
  if (method == properties.end() || method->second != "iFOD2")
    return;

  // iFOD2 records the per-step sample count and the downsample factor that was applied on output.
  //   Each integration step is a single analytically computed arc sampled at samples_per_step points
  //   (i.e. samples_per_step - 1 sub-step segments); the default downsample factor equals that
  //   segment count, leaving one vertex per step. Only when the factor has been reduced do multiple
  //   sub-step vertices per arc survive into the exported streamline.
  const auto samples = properties.find("samples_per_step");
  const auto downsample = properties.find("downsample_factor");
  if (samples == properties.end() || downsample == properties.end())
    return;
  const size_t num_samples = to<size_t>(samples->second);
  const size_t downsample_factor = to<size_t>(downsample->second);
  if (num_samples < 2 || downsample_factor < 1)
    return;
  const default_type grouping =
      static_cast<default_type>(num_samples - 1) / static_cast<default_type>(downsample_factor);
  if (!(grouping > 1.0))
    return;

  config.vertices_per_parent_arc = grouping;
  WARN("input tractogram was generated by iFOD2 with reduced streamline downsampling"
       " (samples_per_step=" +
       str(num_samples) + ", downsample_factor=" + str(downsample_factor) +
       "); curvature scale calibration adjusted to treat each run of " + str(grouping) +
       " sub-step vertices as one independent geometric sample");
}

} // namespace MR::DWI::Tractography
