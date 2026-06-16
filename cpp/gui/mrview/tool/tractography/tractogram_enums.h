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

#include <cstdint>

namespace MR::GUI::MRView::Tool {
enum class TrackColourType { Direction, Ends, Manual, ScalarFile };
enum class TrackGeometryType { Pseudotubes, Lines, Points };
enum class TrackThresholdType { None, UseColourFile, SeparateFile };

//! Per-vertex position within its parent streamline.
/*! Uploaded as a vertex attribute so that the vertex shader can compute the
 *  local tangent without relying on duplicate endpoint padding: the \c prev /
 *  \c next neighbour fetched across a streamline (or buffer) boundary is simply
 *  not used. Values are fixed (an integer vertex attribute compared in GLSL). */
enum class TrackVertexType : uint8_t { Single = 0, First = 1, Middle = 2, Last = 3 };
} // namespace MR::GUI::MRView::Tool
