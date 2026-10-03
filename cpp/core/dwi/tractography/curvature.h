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

#include <vector>

#include "types.h"

#include "dwi/tractography/spline.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

class Properties;

//! Algorithm used to estimate smooth per-vertex curvature.
enum class CurvatureMethod {
  SAVITZKY_GOLAY, //!< Local polynomial-in-arc-length LS fit; analytic r', r''. (default)
  GAUSSIAN_DERIV  //!< Arc-length Gaussian-weighted local polynomial fit; r', r''.
};

//! How the arc-length smoothing scale (mm) is chosen.
enum class CurvatureScale {
  AUTO, //!< Data-driven from the turn-angle autocorrelation length (see curvature.cpp). (default)
  FIXED //!< Use the caller-supplied fixed scale in mm.
};

//! Parameters controlling curvature estimation; every field carries a sensible default.
/*! The smoothing scale is interpreted as the Savitzky-Golay half-window length (mm), or, for
 *  the Gaussian-weighted variant, the Gaussian sigma (mm); both methods share one weighted
 *  local-polynomial core. */
struct CurvatureConfig {
  CurvatureMethod method{CurvatureMethod::SAVITZKY_GOLAY};
  CurvatureScale scale{CurvatureScale::AUTO};
  //! Smoothing scale in mm; used when scale == FIXED, and as the fallback when AUTO cannot
  //!   resolve a scale (very short streamlines).
  default_type fixed_scale_mm{5.0};
  //! Polynomial order for the local fit (must be >= 2 to admit a non-trivial second derivative).
  size_t polynomial_order{2};
  //! Number of consecutive exported vertices that originate from a single deterministically-smooth
  //!   parent arc (e.g. the sub-step samples of one iFOD2 integration step when the default output
  //!   downsampling has been disabled). The AUTO scale tuning treats this many vertices as one
  //!   independent geometric sample, both when normalising the smoothing scale and when estimating
  //!   the turn-angle autocorrelation, so the within-arc determinism is not mistaken for either
  //!   anatomical smoothness or noise. 1.0 (the default) disables the adjustment; populate from the
  //!   input tractogram metadata via configure_from_properties().
  default_type vertices_per_parent_arc{1.0};
};

//! Adjust \c config from input tractogram generator metadata, warning once when an adjustment applies.
/*! Inspects the tractography \c Properties for evidence that contiguous runs of exported vertices are
 *  sub-step samples of a single analytically computed arc (presently: the iFOD2 algorithm with the
 *  default streamline downsampling reduced, so more than one vertex per integration step survives).
 *  When so, sets \c config.vertices_per_parent_arc to the number of such vertices per parent arc and
 *  issues a single terminal warning; otherwise leaves \c config unchanged. Intended to be called once
 *  per command (single-threaded), so the warning fires at most once.
 *
 *  \warning The decision trusts the metadata. If intermediate processing has changed the vertex
 *  spacing without updating the \c downsample_factor / \c samples_per_step fields (for example a
 *  prior resampling to a fixed step size), the grouping could be misapplied; the emitted warning
 *  makes any such adjustment visible. */
void configure_from_properties(CurvatureConfig &config, const Properties &properties);

//! Smooth per-vertex curvature (1/mm) along a streamline.
/*! Returns a vector of length \c tck.size(); element \c i is the estimated curvature magnitude
 *  \f$\kappa = \lVert r' \times r'' \rVert / \lVert r' \rVert^3\f$ at vertex \c i, in 1/mm,
 *  computed from arc-length-smoothed first and second derivatives of the streamline geometry.
 *
 *  Endpoint handling: the fitting window is truncated (one-sided) at the streamline ends; no
 *  ghost vertices are introduced (the reflected ghost defines a zero-curvature continuation,
 *  which would bias the endpoints toward zero).
 *
 *  Degenerate cases:
 *   - \c tck.size() < 3 : curvature is undefined; every element is set to 0.0.
 *   - zero-length steps : coincident vertices are collapsed before fitting; if fewer than 3
 *     distinct vertices remain, every element is set to 0.0.
 *   - \f$\lVert r' \rVert\f$ underflow at a vertex : that element is set to 0.0
 *     (straight / locally stationary), never NaN/Inf.
 *
 *  Complexity: O(N) arc-length accumulation + O(N*W) windowed fitting, with W the number of
 *  vertices spanned by the smoothing scale. No O(N^2)/O(N^3) work. */
std::vector<default_type> curvature(const Streamline<> &tck, const CurvatureConfig &config = CurvatureConfig());

//! Overload reusing a caller-owned SplineView (stage 3.2) when one already exists.
/*! Equivalent to the \c Streamline<> overload; the view is consulted only for its underlying
 *  vertices and size (the smoothing operates on raw vertices and their cumulative arc length,
 *  not on Hermite weights). */
std::vector<default_type> curvature(const SplineView<> &view, const CurvatureConfig &config = CurvatureConfig());

} // namespace MR::DWI::Tractography
