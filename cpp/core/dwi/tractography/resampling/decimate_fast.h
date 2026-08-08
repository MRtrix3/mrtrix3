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

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/resampling/resampling.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Resampling {

//! Fast curvature-adaptive a-priori streamline decimation.
/*! Places a length- and curvature-dependent number of vertices along each streamline in a
 *  single O(N) pass, with no per-streamline search or optimisation. Output vertices lie on the
 *  original tension-Catmull-Rom spline (an on-curve subset): the chosen parameter positions are
 *  evaluated through the same reflected-ghost \c SplineView used by every other consumer, so the
 *  decode-time reconstruction is consistent by construction.
 *
 *  A cost density \f$\rho(s) = 1 + \lambda\,g(\kappa(s))\f$ is integrated along arc length; the
 *  output vertices equidistribute the cumulative cost \f$C(s)\f$, clustering where curvature is
 *  high. The target vertex count is \f$n = \mathrm{clamp}(\mathrm{round}(\mu\,C_{\mathrm{total}}),
 *  2, N)\f$, with the user knob \f$\mu\f$ controlling vertices per unit curvature-weighted arc
 *  length. Both endpoints are retained immutably and the operation never upsamples. */
class DecimateFast : public BaseCRTP<DecimateFast> {

public:
  DecimateFast() : density(0.0) {}
  //! Construct with the user density knob mu (vertices per unit curvature-weighted arc length).
  explicit DecimateFast(const default_type mu) : density(mu) {}

  bool operator()(const Streamline<> &, Streamline<> &) const override;
  bool valid() const override { return density > 0.0; }

  default_type get_density() const { return density; }
  void set_density(const default_type mu) { density = mu; }

  //! Curvature-estimation parameters, normally left at their defaults; the caller may inject a
  //!   metadata-derived configuration (see configure_from_properties()) so that sub-step-sampled
  //!   inputs are handled correctly by the AUTO scale tuning.
  const CurvatureConfig &get_curvature_config() const { return curv_config; }
  void set_curvature_config(const CurvatureConfig &config) { curv_config = config; }

private:
  //! Target vertices per unit curvature-weighted arc length (the user knob mu).
  default_type density;
  //! Curvature-estimation parameters passed through to every per-streamline curvature() call.
  CurvatureConfig curv_config;
};

} // namespace MR::DWI::Tractography::Resampling
