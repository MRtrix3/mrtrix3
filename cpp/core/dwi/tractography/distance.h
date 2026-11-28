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

#include <optional>

#include "types.h"

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/resampling/resampling.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

//! Parameters governing the spline Hausdorff computation.
/*! Every field carries a sensible default. The only inputs the caller usually needs to supply are
 *  the optional Hausdorff threshold (\c threshold_mm) when only an above/below decision is required,
 *  and the spline tension (defaults to the canonical \c Resampling::hermite_tension = 0.1, matching
 *  the reconstruction operator every other consumer uses). */
struct HausdorffConfig {
  //! Catmull-Rom tension; must match the operator the splines are interpreted against.
  default_type tension{static_cast<default_type>(Resampling::hermite_tension)};
  //! Optional Hausdorff threshold (mm). When set it drives the probe-spacing rule (\c tau in the
  //!   derivation below); when unset a small fraction of the inter-vertex spacing is used instead.
  std::optional<default_type> threshold_mm{};
  //! Curvature-estimation parameters used to derive the robust minimum radius of curvature that
  //!   bounds the probe spacing. Normally defaulted; a metadata-derived configuration may be supplied
  //!   so sub-step-sampled inputs are handled correctly by the AUTO scale tuning (see
  //!   configure_from_properties()).
  CurvatureConfig curvature{};
};

//! Result of a (symmetric or directed) spline Hausdorff computation.
/*! \c distance is the (symmetric) Hausdorff distance in mm. \c argmax_global_parameter locates the
 *  probe attaining that distance, as a global spline parameter \c s = segment + mu on whichever
 *  streamline that probe was sampled from; \c argmax_on_first is true when the attaining probe lay
 *  on the first argument (the A->B sweep), false when it lay on the second (the B->A sweep). These
 *  diagnostics are consumed by the stage-3.7 calibration tooling. */
struct HausdorffResult {
  default_type distance;
  default_type argmax_global_parameter;
  bool argmax_on_first;
};

//! Symmetric Hausdorff distance (mm) between the Catmull-Rom splines of two streamlines.
/*! Both streamlines are interpreted as continuous tension Catmull-Rom splines (via the stage-3.2
 *  ghost view and the stage-3.1 augmented Hermite interpolator) and the symmetric Hausdorff
 *  distance between those two curves is returned.
 *
 *  \par Assumptions (this initial version)
 *  The two streamlines are assumed to be \b proximal along their entire lengths and their vertex
 *  orders \b not reversed with respect to one another. These permit a monotone marching
 *  nearest-point search with warm starts (O(1) amortised per probe) rather than a global all-pairs
 *  search. The stage-3.7 caller feeds only original-vs-decimated pairs, which satisfy both by
 *  construction. Relaxing them (crossing / antiparallel / non-proximal streamlines) would require
 *  replacing the marching search with a global or Frechet-style search; the implementation marks
 *  the single site where that substitution would go.
 *
 *  \par Method
 *  A directed distance d(A->B) upsamples A's spline to probe points and, for each probe, finds the
 *  nearest point on B's spline by Newton-Raphson on the foot-point condition
 *  \f$f(s)=(Q-P_B(s))\cdot P_B'(s)=0\f$, warm-started from the previous probe's converged foot. The
 *  symmetric distance is \f$\max(d(A\to B), d(B\to A))\f$. The probe spacing is chosen by a
 *  curvature-aware rule (see distance.cpp) so the chord-vs-arc discretisation error is far below the
 *  quantity of interest.
 *
 *  \par Degenerate inputs
 *  Streamlines with fewer than two vertices, or that collapse to a single distinct point, are
 *  handled by falling back to point/endpoint distances; the result is always finite (never NaN). */
HausdorffResult
hausdorff(const Streamline<> &a, const Streamline<> &b, const HausdorffConfig &config = HausdorffConfig());

} // namespace MR::DWI::Tractography
