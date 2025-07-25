/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "denoise/kernel/data.h"
#include "denoise/kernel/sphere_base.h"
#include "header.h"

namespace MR::Denoise::Kernel {

constexpr default_type sphere_multiplier_default = 1.0 / 0.85;

class SphereRatio : public SphereBase {

public:
  SphereRatio(const Header &voxel_grid, const std::array<ssize_t, 3> &subsample_factors, const default_type min_ratio)
      : SphereBase(voxel_grid, subsample_factors, compute_max_radius(voxel_grid, min_ratio)),
        min_size(std::ceil(voxel_grid.size(3) * min_ratio)) {}

  SphereRatio(const SphereRatio &) = default;

  ~SphereRatio() override = default;

  Data operator()(const Voxel::index_type &pos) const override;

  ssize_t estimated_size() const override { return min_size; }

private:
  ssize_t min_size;

  // Determine an appropriate bounding box from which to generate the search table
  // Find the radius for which 7/8 of the sphere will contain the minimum number of voxels, then round up
  // This is only for setting the maximal radius for generation of the lookup table
  default_type compute_max_radius(const Header &voxel_grid, const default_type min_ratio) const;
};

} // namespace MR::Denoise::Kernel
