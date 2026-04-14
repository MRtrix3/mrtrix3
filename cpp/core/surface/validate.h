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

namespace MR::Surface {

class Mesh;
class MeshMulti;

//! Validate a mesh surface against a set of topological and geometric requirements.
//!
//! Checks (in order):
//!   1. No disconnected vertices: every vertex is referenced by at least one polygon.
//!   2. No duplicate vertices: no two vertices share identical 3D positions.
//!   3. No duplicate polygons: no two polygons reference the same set of vertex indices.
//!   4. Closed surface: every edge is shared by exactly two polygons (no boundary or
//!      non-manifold edges).
//!   5. Single connected component: all polygons belong to one connected surface.
//!   6. Consistent normals: for every shared edge, the two adjacent polygons traverse it
//!      in opposite directions (ensuring a consistent normal orientation).
//!
//! Throws Exception with a descriptive message on the first violated requirement.
void validate(const Mesh &mesh);

//! Call validate_mesh() only when running in debug mode (log_level >= 3).
//! Intended for use in other mesh-processing commands to add validation in debug builds.
void debug_validate(const Mesh &mesh);
void debug_validate(const MeshMulti &meshes);

} // namespace MR::Surface
