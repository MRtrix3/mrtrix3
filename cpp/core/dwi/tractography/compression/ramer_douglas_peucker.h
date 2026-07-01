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

#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Compression {

//! \brief Endpoint-preserving Ramer–Douglas–Peucker (RDP) streamline simplification.
/*! Implements the "piece-wise linearization" step of the .zfib compression
 * pipeline (Presseau et al., Eq. 1): given a tolerance \a tolerance_mm, the
 * routine removes as many interior vertices as possible while guaranteeing that
 * every removed vertex lies within \a tolerance_mm of the retained polyline.
 * The first and last vertices are always preserved, which is essential for
 * tractography: end-point accuracy governs the connectivity profile (§ paper).
 *
 * This differs from Resampling::Downsampler, which decimates at a fixed ratio
 * irrespective of geometry; RDP is error-bounded, so the number of retained
 * vertices adapts to local curvature. The routine is placed in a reusable
 * \c compression/ component so that tckedit / tckresample may adopt it later. */

//! \brief Indices (into \a tck) of the vertices RDP retains for \a tolerance_mm.
/*! The returned indices are strictly increasing and always include 0 and
 * tck.size()-1 (when non-empty). A streamline of fewer than three vertices has
 * no interior point to drop and is returned unchanged (all indices). The
 * implementation uses an explicit stack rather than recursion to bound stack
 * usage on pathologically long streamlines.
 *
 * The split predicate uses a strict comparison (split when the farthest interior
 * vertex's perpendicular distance is strictly greater than \a tolerance_mm); this
 * is flagged as a tunable for byte-exact interop against the reference encoder. */
template <class ValueType>
std::vector<size_t> rdp_retained_indices(const Streamline<ValueType> &tck, ValueType tolerance_mm);

//! \brief The linearized copy of \a tck retaining only the RDP indices.
/*! Preserves the streamline's \c weight and ordering index. A streamline of
 * fewer than three vertices is returned unchanged. */
template <class ValueType> Streamline<ValueType> linearize(const Streamline<ValueType> &tck, ValueType tolerance_mm);

} // namespace MR::DWI::Tractography::Compression
