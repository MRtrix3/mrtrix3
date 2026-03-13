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

#include "image.h"
#include "misc/cuboid_extent.h"
#include "types.h"

namespace MR::Adapter {

template <class ImageType> class NeighbourhoodCoord : public Base<NeighbourhoodCoord<ImageType>, ImageType> {
public:
  using base_type = Base<NeighbourhoodCoord<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::name;
  using base_type::spacing;

  NeighbourhoodCoord(const ImageType &original, const CuboidExtent &extent, const Iterator &iter)
      : base_type(original), iter_(iter), transform_(original.transform()) {

    assert(extent.size() == ndim());
    from_.resize(original.ndim());
    size_.resize(original.ndim());
    for (ArrayIndex i = 0; i < ndim(); ++i) {
      from_[i] = (iter_.index(i) - extent[i] < 0) ? 0 : iter_.index(i) - extent[i];
      size_[i] = (from_[i] + extent[i] >= original.size(i)) ? original.size(i) - from_[i] - 1 : extent[i];
      assert(from_[n] + size_[n] < original.size(n));
    }

    // for (size_t n = 0; n < ndim(); ++n)
    //   if (from_[n] + size_[n] > original.size(n))
    //     throw Exception ("FIXME: dimensions requested for NeighbourhoodCoord adapter are out of bounds!");

    for (ArrayIndex j = 0; j < 3; ++j)
      for (ArrayIndex i = 0; i < 3; ++i)
        transform_(i, 3) += from_[j] * spacing(j) * transform_(i, j);
  }

  void reset() {
    for (ArrayIndex n = 0; n < ndim(); ++n)
      set_pos(n, 0);
  }

  size_t ndim() const override { return size_.size(); }
  size_t size(const ArrayIndex axis) const override { return size_[axis]; }
  const transform_type &transform() const override { return transform_; }

  VoxelIndex get_index(const ArrayIndex axis) const override { return parent().index(axis) - from_[axis]; }
  void move_index(const ArrayIndex axis, const VoxelIndex increment) { parent().index(axis) += increment; }

protected:
  using base_type::parent;
  std::vector<VoxelIndex> from_, size_;
  Iterator iter_;
  transform_type transform_;
};

} // namespace MR::Adapter
