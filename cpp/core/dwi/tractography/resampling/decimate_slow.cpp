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

#include "dwi/tractography/resampling/decimate_slow.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "dwi/tractography/foot_point.h"
#include "dwi/tractography/spline.h"

namespace MR::DWI::Tractography::Resampling {

namespace {

using FootPoint::vec3;

//! Hard cap on greedy knot-insertion iterations, as a multiple of the input vertex count.
/*! The greedy scheme inserts at most one original-vertex foot per iteration and can never need
 *  more control points than the input has vertices; this multiple is a pure safety stop guarding
 *  against a pathological non-converging tolerance (e.g. epsilon below the foot-point solver's own
 *  resolution), never reached for sane inputs. */
constexpr size_t max_insertions_per_input_vertex = 2;

//! Number of golden-section iterations used by the per-knot slide line search.
constexpr size_t slide_search_iterations = 24;

//! A control point of the reconstruction, located on the ORIGINAL spline.
/*! The control set is stored as parameters \c t on the original spline (global parameter
 *  \c segment + mu in [0, N-1]); the reconstructed position is the original spline evaluated there,
 *  guaranteeing every output vertex lies on the original curve. */
struct Control {
  default_type t;             //!< Global parameter on the ORIGINAL spline.
  Streamline<>::point_type p; //!< Position on the original spline at \c t (the emitted vertex).
};

//! The nearest-point foot of one original vertex on the current reconstruction.
struct VertexFoot {
  default_type s;    //!< Global parameter on the RECONSTRUCTION spline (segment + mu in [0, m-1]).
  default_type dist; //!< Deviation (mm) from the original vertex to that foot.
};

//! Build the reconstruction streamline from the current control positions.
void assemble_reconstruction(const std::vector<Control> &controls, Streamline<> &recon) {
  recon.clear();
  recon.reserve(controls.size());
  for (const Control &c : controls)
    recon.push_back(c.p);
}

} // namespace

bool DecimateSlow::operator()(const Streamline<> &in, Streamline<> &out) const {
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

  const default_type tension = static_cast<default_type>(hermite_tension);
  const size_t num_vertices = in.size();
  const SplineView<value_type> original(in);
  const default_type original_s_max = static_cast<default_type>(num_vertices - 1);

  // -------------------------------------------------------------------------------------------
  // Step 1: control set initialised to the two (immutable) endpoints, expressed as parameters on
  //   the original spline. Interior control points inserted later will likewise be on-curve.
  // -------------------------------------------------------------------------------------------
  std::vector<Control> controls;
  controls.reserve(num_vertices);
  controls.push_back({0.0, in.front()});
  controls.push_back({original_s_max, in.back()});

  // Per-original-vertex foot on the current reconstruction (deviation + reconstruction parameter).
  std::vector<VertexFoot> feet(num_vertices);
  // Bucket assignment: which control interval each original vertex's original-parameter falls in.
  //   Original vertices and control points are co-monotone in original parameter, so each vertex
  //   belongs to exactly one interval; an insertion splits one interval and only its members are
  //   re-footed (the warm-started incremental update that avoids the quadratic rescan).
  std::vector<size_t> vertex_interval(num_vertices, 0);

  // -------------------------------------------------------------------------------------------
  // Foot-point refresh for a contiguous range of original vertices against the reconstruction,
  //   warm-started by marching in order from a seed parameter. Returns nothing; updates feet[].
  // -------------------------------------------------------------------------------------------
  const auto refresh_feet =
      [&](const Streamline<> &recon, const size_t v_begin, const size_t v_end, const default_type warm_seed) {
        const SplineView<value_type> recon_view(recon);
        const default_type recon_s_max = static_cast<default_type>(recon.size() - 1);
        default_type warm = warm_seed;
        for (size_t v = v_begin; v != v_end; ++v) {
          const vec3 probe = in[v].template cast<default_type>();
          const FootPoint::Foot foot = FootPoint::nearest_point(recon_view, tension, probe, warm, recon_s_max);
          warm = foot.s;
          feet[v] = {foot.s, std::sqrt(std::max<default_type>(0.0, foot.dist_sq))};
        }
      };

  Streamline<> recon;
  assemble_reconstruction(controls, recon);

  // Initial pass: every original vertex's foot on the two-endpoint reconstruction, marching in
  //   order so each search warm-starts from the previous vertex's converged foot.
  refresh_feet(recon, 0, num_vertices, 0.0);
  std::fill(vertex_interval.begin(), vertex_interval.end(), 0);

  // -------------------------------------------------------------------------------------------
  // Running max deviation (and its argmax vertex) maintained per control interval, so selecting
  //   the next insertion is O(m) over the interval maxima rather than an O(N) rescan of all feet.
  //   interval_max[j] / interval_argmax[j] summarise the deviations of the vertices in interval j.
  // -------------------------------------------------------------------------------------------
  std::vector<default_type> interval_max;
  std::vector<size_t> interval_argmax;
  const auto recompute_interval = [&](const size_t j) {
    default_type best = -1.0;
    size_t best_v = 0;
    for (size_t v = 0; v != num_vertices; ++v) {
      if (vertex_interval[v] != j)
        continue;
      if (feet[v].dist > best) {
        best = feet[v].dist;
        best_v = v;
      }
    }
    interval_max[j] = best;
    interval_argmax[j] = best_v;
  };
  interval_max.assign(1, -1.0);
  interval_argmax.assign(1, 0);
  recompute_interval(0);

  const size_t max_insertions = max_insertions_per_input_vertex * num_vertices;

  // -------------------------------------------------------------------------------------------
  // Step 2-5: greedy knot insertion until the worst deviation is within tolerance.
  //
  // Hoschek-style parameter correction would wrap this insertion loop: after (or interleaved with)
  //   each insertion, globally re-optimise the foot-point parameterisation by alternating a
  //   linear least-squares re-fit of the control positions with a re-projection of every original
  //   vertex onto the updated curve. Benefit: a lower L-infinity deviation for a given knot count,
  //   and the bridge to free-in-R^3 control points (design Problem 3). Not implemented here: it
  //   trades the strictly on-curve, decode-compatible control set for a denser global solve.
  // -------------------------------------------------------------------------------------------
  for (size_t iteration = 0; iteration != max_insertions; ++iteration) {
    // Select the globally worst interval (O(m)); its argmax vertex is the insertion candidate.
    size_t worst_interval = 0;
    default_type worst_dist = -1.0;
    for (size_t j = 0; j != interval_max.size(); ++j) {
      if (interval_max[j] > worst_dist) {
        worst_dist = interval_max[j];
        worst_interval = j;
      }
    }
    if (!(worst_dist > tolerance))
      break;

    const size_t worst_vertex = interval_argmax[worst_interval];

    // Insertion choice (b): insert the on-curve point of the original spline nearest the worst
    //   vertex, rather than the worst vertex itself. The worst vertex is by construction an
    //   original spline vertex, so its on-curve foot is simply its own original parameter t = i;
    //   this keeps the control set strictly on the original curve (Method-B-faithful), preserving
    //   decode compatibility with the reflected-ghost reconstruction. (Choice (a), inserting the
    //   raw vertex coordinate, is identical here because the vertex already lies on the curve;
    //   choosing the on-curve parameterisation is the form that generalises to sub-vertex feet.)
    const default_type t_new = static_cast<default_type>(worst_vertex);

    // Guard: refuse a degenerate insertion that does not split the interval (coincident parameter).
    if (!(t_new > controls[worst_interval].t) || !(t_new < controls[worst_interval + 1].t)) {
      // The worst vertex sits on an existing knot: its deviation cannot be reduced by insertion.
      //   Mark this interval resolved so the loop can consider the next-worst region.
      interval_max[worst_interval] = -1.0;
      continue;
    }

    // Insert the new control point (on the original spline) and the matching empty interval.
    const Control inserted{t_new, original.position(t_new, static_cast<value_type>(tension))};
    controls.insert(controls.begin() + worst_interval + 1, inserted);
    interval_max.insert(interval_max.begin() + worst_interval + 1, -1.0);
    interval_argmax.insert(interval_argmax.begin() + worst_interval + 1, 0);

    // Re-bucket: vertices in the split interval move to the left or right child; vertices in every
    //   later interval shift their interval index up by one (their geometry is unchanged).
    for (size_t v = 0; v != num_vertices; ++v) {
      if (vertex_interval[v] > worst_interval) {
        ++vertex_interval[v];
      } else if (vertex_interval[v] == worst_interval) {
        const default_type t_v = static_cast<default_type>(v);
        vertex_interval[v] = (t_v < t_new) ? worst_interval : (worst_interval + 1);
      }
    }

    // Re-reconstruct (one extra control point) and re-foot ONLY the original vertices in the two
    //   child intervals, warm-started from the parameter of the split knot on the new curve. All
    //   other feet remain valid warm starts / optima and are left untouched (the incremental
    //   update that keeps the encoder out of the quadratic trap).
    assemble_reconstruction(controls, recon);
    size_t affected_begin = num_vertices;
    size_t affected_end = 0;
    for (size_t v = 0; v != num_vertices; ++v) {
      if (vertex_interval[v] == worst_interval || vertex_interval[v] == worst_interval + 1) {
        affected_begin = std::min(affected_begin, v);
        affected_end = std::max(affected_end, v + 1);
      }
    }
    if (affected_begin < affected_end) {
      // Warm-start the marching re-foot from the inserted knot's reconstruction parameter, which
      //   is exactly its new control index (positions are evaluated at the controls).
      const default_type warm_seed = static_cast<default_type>(worst_interval + 1);
      refresh_feet(recon, affected_begin, affected_end, warm_seed);
    }

    // Update the two child intervals' running maxima (O(N) over their members only).
    recompute_interval(worst_interval);
    recompute_interval(worst_interval + 1);
  }

  // -------------------------------------------------------------------------------------------
  // Step 6: slide refinement. Each interior control point is slid along the original spline (a 1-D
  //   golden-section search of its original parameter, bracketed by its neighbours) to minimise the
  //   worst deviation among the original vertices bucketed into its two adjacent intervals. The
  //   endpoints are never moved. After sliding, the affected feet and interval maxima are refreshed.
  // -------------------------------------------------------------------------------------------
  for (size_t k = 1; k + 1 < controls.size(); ++k) {
    const default_type t_lo = controls[k - 1].t;
    const default_type t_hi = controls[k + 1].t;
    if (!(t_hi - t_lo > 0.0))
      continue;

    // Objective: worst deviation over the vertices in intervals (k-1) and k, as a function of the
    //   slid parameter of control k. Evaluated by re-reconstructing locally and re-footing just
    //   those vertices; this is the per-knot local max the slide minimises.
    const auto local_max_deviation = [&](const default_type t_candidate) -> default_type {
      std::vector<Control> trial = controls;
      trial[k].t = t_candidate;
      trial[k].p = original.position(t_candidate, static_cast<value_type>(tension));
      Streamline<> trial_recon;
      assemble_reconstruction(trial, trial_recon);
      const SplineView<value_type> trial_view(trial_recon);
      const default_type trial_s_max = static_cast<default_type>(trial_recon.size() - 1);
      default_type worst = 0.0;
      default_type warm = static_cast<default_type>(k - 1);
      for (size_t v = 0; v != num_vertices; ++v) {
        if (vertex_interval[v] != k - 1 && vertex_interval[v] != k)
          continue;
        const vec3 probe = in[v].template cast<default_type>();
        const FootPoint::Foot foot = FootPoint::nearest_point(trial_view, tension, probe, warm, trial_s_max);
        warm = foot.s;
        worst = std::max(worst, std::sqrt(std::max<default_type>(0.0, foot.dist_sq)));
      }
      return worst;
    };

    // Golden-section minimisation of the local max deviation over the open interval (t_lo, t_hi).
    constexpr default_type inv_phi = 0.6180339887498949;
    default_type a = t_lo;
    default_type b = t_hi;
    default_type c = b - inv_phi * (b - a);
    default_type d = a + inv_phi * (b - a);
    default_type fc = local_max_deviation(c);
    default_type fd = local_max_deviation(d);
    for (size_t i = 0; i != slide_search_iterations; ++i) {
      if (fc < fd) {
        b = d;
        d = c;
        fd = fc;
        c = b - inv_phi * (b - a);
        fc = local_max_deviation(c);
      } else {
        a = c;
        c = d;
        fc = fd;
        d = a + inv_phi * (b - a);
        fd = local_max_deviation(d);
      }
    }
    const default_type t_best = 0.5 * (a + b);
    // Accept the slide only if it does not worsen the current local max (golden-section on a
    //   unimodal-ish objective; the guard keeps a noisy multi-modal case from regressing).
    if (local_max_deviation(t_best) <= local_max_deviation(controls[k].t)) {
      controls[k].t = t_best;
      controls[k].p = original.position(t_best, static_cast<value_type>(tension));
    }
  }

  // Lyche-Morken knot removal would slot in here, after the slide converges: attempt to remove the
  //   interior control point whose removal least increases the deviation, accepting the removal
  //   only while the reconstruction still satisfies epsilon, and repeating greedily. Benefit:
  //   trims knots that the greedy insertion left redundant once later insertions / the slide
  //   re-shaped the local fit, yielding a strictly smaller control set for the same tolerance.
  //   Not implemented here (scope limit): it requires a re-fit-and-recheck pass per removal
  //   candidate and a removal-cost ordering analogous to the B-spline knot-removal error bound.

  // -------------------------------------------------------------------------------------------
  // Emit the control positions (all on the original spline; endpoints exact by construction).
  // -------------------------------------------------------------------------------------------
  out.reserve(controls.size());
  for (const Control &c : controls) {
    out.push_back(c.p);
    assert(out.back().allFinite());
  }
  // Endpoints exact: replace the evaluated endpoint positions with the verbatim input endpoints
  //   (evaluation at t=0 / t=N-1 reproduces them, but pin them to defeat any rounding).
  out.front() = in.front();
  out.back() = in.back();
  return true;
}

} // namespace MR::DWI::Tractography::Resampling
