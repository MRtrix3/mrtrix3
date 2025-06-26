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
#include "types.h"

namespace MR::Denoise::Kernel {

class SphereFixedRadius : public SphereBase {
public:
  SphereFixedRadius(const Header &voxel_grid,                         //
                    const std::array<ssize_t, 3> &subsample_factors,  //
                    const default_type radius)                        //
      : SphereBase(voxel_grid, subsample_factors, radius),            //
        maximum_size(std::distance(shared->begin(), shared->end())) { //
    INFO("Maximum number of voxels in " + str(radius) + "mm fixed-radius kernel is " + str(maximum_size));
  }
  SphereFixedRadius(const SphereFixedRadius &) = default;
  ~SphereFixedRadius() override = default;
  Data operator()(const Voxel::index_type &pos) const override;
  ssize_t estimated_size() const override { return maximum_size; }

private:
  const ssize_t maximum_size;
};

} // namespace MR::Denoise::Kernel
