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

#include "dwi/tractography/resampling/resampling.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Resampling {

//! Slow near-minimal streamline decimation by greedy knot insertion with slide refinement.
/*! Produces an on-curve control set whose tension-Catmull-Rom reconstruction stays within a
 *  user-supplied tolerance \f$\varepsilon\f$ (mm) of the original spline at every original vertex,
 *  using as few vertices as the greedy scheme can manage (design Problem 2 / Method B). Both
 *  endpoints are retained immutably and exactly; every interior control point lies \b on the
 *  original spline (insertion at the on-curve foot of the worst-deviating vertex), so the
 *  decode-time reconstruction through the same reflected-ghost \c SplineView is consistent by
 *  construction with the fast decimator and every other consumer.
 *
 *  \par Algorithm
 *  Start from the two endpoints; reconstruct; for each original vertex find its nearest point on
 *  the current reconstruction (warm-started Gauss-Newton foot-point solve, stage 3.5 machinery);
 *  if the worst deviation exceeds \f$\varepsilon\f$ insert one new on-curve control point at the
 *  foot of the worst vertex and repeat; once converged, slide each interior control point along
 *  the original spline to reduce its local max deviation. The per-vertex feet are carried across
 *  iterations and only those in the arc adjacent to a freshly inserted knot are recomputed, so the
 *  encoder avoids the quadratic per-iteration rescan and runs in roughly
 *  \f$O((N + m)\,\rho)\f$ for \f$N\f$ input vertices, \f$m\f$ output vertices and \f$\rho\f$
 *  Newton iterations per foot. */
class DecimateSlow : public BaseCRTP<DecimateSlow> {

public:
  DecimateSlow() : tolerance(0.0) {}
  //! Construct with the deviation tolerance epsilon (mm).
  explicit DecimateSlow(const default_type epsilon) : tolerance(epsilon) {}

  bool operator()(const Streamline<> &, Streamline<> &) const override;
  bool valid() const override { return tolerance > 0.0; }

  default_type get_tolerance() const { return tolerance; }
  void set_tolerance(const default_type epsilon) { tolerance = epsilon; }

private:
  //! Maximum permitted deviation (mm) of the reconstruction from the original spline.
  default_type tolerance;
};

} // namespace MR::DWI::Tractography::Resampling
