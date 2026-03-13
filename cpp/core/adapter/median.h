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

#include <array>
#include <vector>

#include "math/median.h"
#include "misc/cuboid_extent.h"
#include "types.h"

namespace MR::Adapter {

template <class ImageType> class Median : public Base<Median<ImageType>, ImageType> {
public:
  using base_type = Base<Median<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;
  using voxel_type = Median;

  using base_type::index;
  using base_type::name;
  using base_type::size;

  Median(const ImageType &parent) : base_type(parent), extent(3) { update_halfextent(); }

  Median(const ImageType &parent, const CuboidExtent &extent) : base_type(parent), extent(extent) {
    update_halfextent();
  }

  void set_extent(const CuboidExtent &ext) {
    extent = ext;
    update_halfextent();
  }

  value_type value() {
    const std::array<VoxelIndex, 3> old_pos = {index(0), index(1), index(2)};
    const std::array<VoxelIndex, 3> from = {index(0) < halfextent[0] ? 0 : index(0) - halfextent[0],
                                            index(1) < halfextent[1] ? 0 : index(1) - halfextent[1],
                                            index(2) < halfextent[2] ? 0 : index(2) - halfextent[2]};
    const std::array<VoxelIndex, 3> to = {
        index(0) >= size(0) - halfextent[0] ? static_cast<VoxelIndex>(size(0)) : index(0) + halfextent[0] + 1,
        index(1) >= size(1) - halfextent[1] ? static_cast<VoxelIndex>(size(1)) : index(1) + halfextent[1] + 1,
        index(2) >= size(2) - halfextent[2] ? static_cast<VoxelIndex>(size(2)) : index(2) + halfextent[2] + 1};

    values.clear();

    for (index(2) = from[2]; index(2) < to[2]; ++index(2))
      for (index(1) = from[1]; index(1) < to[1]; ++index(1))
        for (index(0) = from[0]; index(0) < to[0]; ++index(0))
          values.push_back(base_type::value());

    index(0) = old_pos[0];
    index(1) = old_pos[1];
    index(2) = old_pos[2];

    return Math::median(values);
  }

protected:
  CuboidExtent extent;
  std::array<VoxelIndex, 3> halfextent;
  std::vector<value_type> values;

  void update_halfextent() {
    for (StdIndex i = 0; i != 3; ++i)
      halfextent[i] = (extent[i] - 1) / 2;
  }
};

} // namespace MR::Adapter
