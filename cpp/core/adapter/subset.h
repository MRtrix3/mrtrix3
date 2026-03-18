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
#include "image.h"

namespace MR::Adapter {

template <class ImageType> class Subset : public Base<Subset<ImageType>, ImageType> {
public:
  using base_type = Base<Subset<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::name;
  using base_type::spacing;

  template <class VectorType>
  Subset(const ImageType &original, const VectorType &from, const VectorType &size)
      : base_type(original),
        from_(container_cast<decltype(from_)>(from)),
        size_(container_cast<decltype(size_)>(size)),
        transform_(original.transform()) {

    for (StdIndex n = 0; n < ndim(); ++n) {
      if (size_[n] < 1)
        throw Exception("FIXME: sizes requested for Subset adapter must be positive");
      if (from_[n] + size_[n] > original.size(n) || from_[n] < 0)
        throw Exception("FIXME: dimensions requested for Subset adapter are out of bounds!");
    }

    for (ArrayIndex j = 0; j < 3; ++j)
      for (ArrayIndex i = 0; i < 3; ++i)
        transform_(i, 3) += from[j] * spacing(j) * transform_(i, j);
  }

  void reset() override {
    for (ArrayIndex n = 0; n < ndim(); ++n)
      set_pos(n, 0);
  }

  size_t ndim() const override { return size_.size(); }
  VoxelIndex size(const ArrayIndex axis) const override { return size_[axis]; }
  const transform_type &transform() const override { return transform_; }

  VoxelIndex get_index(const ArrayIndex axis) const override { return parent().index(axis) - from_[axis]; }
  void move_index(const ArrayIndex axis, const VoxelIndex increment) override { parent().index(axis) += increment; }

protected:
  using base_type::parent;
  const std::vector<VoxelIndex> from_, size_;
  transform_type transform_;
};

} // namespace MR::Adapter
