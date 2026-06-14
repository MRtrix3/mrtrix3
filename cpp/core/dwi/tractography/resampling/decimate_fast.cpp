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

#include "dwi/tractography/resampling/decimate_fast.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/spline.h"

namespace MR::DWI::Tractography::Resampling {

namespace {

//! Curvature weighting in the cost density rho = 1 + lambda * g(kappa).
/*! Tuned so that a vertex sitting in a region of "typical" white-matter curvature (order
 *  0.1-0.2 1/mm) receives an O(1) cost boost relative to a straight segment; larger values make
 *  the sampler more aggressively curvature-following at the expense of plain arc-length coverage.
 *  Kept as an internal constant rather than a second user option: exposing it alongside mu would
 *  be redundant (mu already scales the total vertex budget) and invites option sprawl, while the
 *  stage-3.7 calibration maps the single knob mu to an mm error target with lambda fixed. */
constexpr default_type curvature_weight = 4.0;

//! Sublinear curvature map g(kappa) = kappa^(1/4).
/*! Motivated by cubic-truncation-error scaling: the pointwise error of cubic Hermite
 *  interpolation over a segment of arc length h behaves as ~|f''''| h^4, so to equalise that
 *  error the local step should satisfy h ~ (eps / |f''''|)^(1/4), i.e. the sampling *density*
 *  (vertices per unit length) should scale as |f''''|^(1/4). Using curvature kappa as the
 *  available proxy for the relevant high-order derivative magnitude gives g(kappa) = kappa^(1/4).
 *  The fourth-root is deliberately gentle: it clusters vertices toward high-curvature regions
 *  without starving the low-curvature remainder of the streamline. */
inline default_type curvature_map(const default_type kappa) {
  return std::pow(std::max<default_type>(kappa, 0.0), 0.25);
}

} // namespace

bool DecimateFast::operator()(const Streamline<> &in, Streamline<> &out) const {
  out.clear();
  if (!valid())
    return false;
  out.set_index(in.get_index());
  out.weight = in.weight;

  // Streamlines that cannot define an interior spline pass through unchanged.
  if (in.size() <= 2) {
    out = in;
    return true;
  }

  const size_t num_vertices = in.size();

  // Per-vertex curvature (1/mm); robust to probabilistic wiggle (stage 3.4). The curvature config
  //   carries any metadata-derived adjustment for sub-step-sampled inputs (configure_from_properties).
  const std::vector<default_type> kappa = curvature(in, curv_config);

  // Cumulative cost C[i] = integral of rho along arc length up to vertex i, with
  //   rho(s) = 1 + lambda * g(kappa(s)). The integral is approximated trapezoidally per segment,
  //   weighting the average of the two endpoint densities by the segment's chord length.
  std::vector<default_type> rho;
  rho.reserve(kappa.size());
  for (size_t i = 0; i != num_vertices; ++i)
    rho.push_back(1.0 + curvature_weight * curvature_map(kappa[i]));
  std::vector<default_type> cost(num_vertices, 0.0);
  for (size_t i = 1; i != num_vertices; ++i) {
    const default_type step = (in[i] - in[i - 1]).norm();
    cost[i] = cost[i - 1] + step * 0.5 * (rho[i - 1] + rho[i]);
  }
  const default_type cost_total = cost[num_vertices - 1];

  // Degenerate (zero-length / coincident) input: emit the two endpoints only, never NaN.
  if (cost_total <= 0.0) {
    out.reserve(2);
    out.push_back(in.front());
    out.push_back(in.back());
    return true;
  }

  // Target vertex count: n vertices per unit curvature-weighted cost, clamped so the result
  //   keeps at least the two endpoints and never exceeds (i.e. never upsamples) the input.
  const size_t n = std::clamp<size_t>(static_cast<size_t>(std::lround(density * cost_total)), 2, num_vertices);

  const SplineView<value_type> view(in);
  out.push_back(in.front());

  // Interior vertices equidistribute the cumulative cost: target_k = k * cost_total / (n - 1).
  //   Each target is inverted against the per-vertex cost array to a global spline parameter
  //   s = segment + mu, then evaluated on the original spline so output points sit on the curve.
  size_t segment = 0;
  for (size_t k = 1; k + 1 != n; ++k) {
    const default_type target = cost_total * static_cast<default_type>(k) / static_cast<default_type>(n - 1);
    while (segment + 1 < num_vertices && cost[segment + 1] < target)
      ++segment;
    const default_type cost_span = cost[segment + 1] - cost[segment];
    const default_type mu = cost_span > 0.0 ? (target - cost[segment]) / cost_span : 0.0;
    const default_type s = static_cast<default_type>(segment) + mu;
    out.push_back(view.position(s, hermite_tension));
    assert(out.back().allFinite());
  }

  out.push_back(in.back());
  return true;
}

} // namespace MR::DWI::Tractography::Resampling
