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

#include "image.h"
#include "surface/mesh.h"
#include "types.h"

namespace MR::Surface::Algo {

//! Algorithm used to compute partial volume fractions from a closed surface mesh
enum class Mesh2ImageMethod {
  //! Geometric voxelisation after Toblerone (Kirk et al., IEEE TMI 2020)
  TOBLERONE,
  //! Legacy dense point-lattice supersampling with heuristic interior classification
  BRUTE_FORCE
};

struct Mesh2ImageOptions {
  Mesh2ImageMethod method = Mesh2ImageMethod::TOBLERONE;
  //! Toblerone only: target sub-voxel edge length in mm; if unset an internal default (0.75mm) is used
  std::optional<default_type> subvoxel_mm;
};

void mesh2image(const Mesh &, Image<float> &, const Mesh2ImageOptions & = {});

} // namespace MR::Surface::Algo
