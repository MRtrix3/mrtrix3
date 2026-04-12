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

#include "adapter/base.h"
#include "stride.h"

namespace MR::Adapter {

template <class ImageType> class PermuteAxes : public Base<PermuteAxes<ImageType>, ImageType> {
public:
  using base_type = Base<PermuteAxes<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::parent;
  using base_type::size;

  PermuteAxes(const ImageType &original, const Stride::Order &axes)
      : base_type(original), axes_(axes), ghost_indices(axes.size(), 0) {
    for (ArrayIndex i = 0; i < parent().ndim(); ++i) {
      for (StdIndex a = 0; a < axes_.size(); ++a) {
        if (axes_[a] >= static_cast<Stride::Order::value_type>(parent().ndim()))
          throw Exception("axis " + str(axes_[a]) + " exceeds image dimensionality");
        if (axes_[a] == i)
          goto next_axis;
      }
      if (parent().size(i) != 1)
        throw Exception("omitted axis \"" + str(i) + "\" has dimension greater than 1");
    next_axis:
      continue;
    }
  }

  size_t ndim() const override { return axes_.size(); }
  VoxelIndex size(const ArrayIndex axis) const override { return axes_[axis] < 0 ? 1 : parent().size(axes_[axis]); }
  default_type spacing(const ArrayIndex axis) const override {
    return axes_[axis] < 0 ? std::numeric_limits<default_type>::quiet_NaN() : parent().spacing(axes_[axis]);
  }
  Stride::Actual::value_type stride(const ArrayIndex axis) const override {
    return axes_[axis] < 0 ? 0 : parent().stride(axes_[axis]);
  }

  VoxelIndex get_index(const ArrayIndex axis) const override {
    const auto a = axes_[axis];
    return a < 0 ? ghost_indices[axis] : parent().index(a);
  }
  void move_index(const ArrayIndex axis, const VoxelIndex increment) override {
    const auto a = axes_[axis];
    if (a < 0)
      ghost_indices[axis] += increment;
    else
      parent().index(a) += increment;
  }

private:
  Stride::Order axes_;
  std::vector<VoxelIndex> ghost_indices;
};

} // namespace MR::Adapter
