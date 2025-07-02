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

#include <array>

#include "denoise/kernel/base.h"
#include "denoise/kernel/data.h"
#include "header.h"

namespace MR::Denoise::Kernel {

class Cuboid : public Base {

public:
  Cuboid(const Header &header, const std::array<ssize_t, 3> &subsample_factors, const std::array<ssize_t, 3> &extent);
  Cuboid(const Cuboid &) = default;
  ~Cuboid() override = default;
  Data operator()(const Voxel::index_type &pos) const override;
  ssize_t estimated_size() const override { return size; }

private:
  Eigen::Array<int, 3, 2> bounding_box;
  const ssize_t size;
  const ssize_t centre_index;
};

} // namespace MR::Denoise::Kernel
