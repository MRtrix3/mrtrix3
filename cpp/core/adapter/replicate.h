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
#include "header.h"
#include "image_helpers.h"
#include "stride.h"

namespace MR::Adapter {

template <class ImageType> class Replicate : public Base<Replicate<ImageType>, ImageType> {
public:
  using base_type = Base<Replicate<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::name;
  using base_type::ndim;
  using base_type::spacing;

  Replicate(ImageType &original, const Header &replication_template)
      : base_type(original),
        header_(replication_template),
        pos_(static_cast<VoxelIndex>(std::max(parent().ndim(), static_cast<size_t>(header_.ndim()))), VoxelIndex(0)) {
    for (ArrayIndex n = 0; n < std::min<ArrayIndex>(parent().ndim(), header_.ndim()); ++n) {
      if (n < parent().ndim())
        parent().index(n) = 0;
      if (parent().size(n) > 1 && parent().size(n) != header_.size(n))
        throw Exception("cannot replicate over non-singleton dimensions");
    }
  }

  size_t ndim() const override { return header_.ndim(); }
  VoxelIndex size(const ArrayIndex axis) const override { return header_.size(axis); }
  default_type spacing(const ArrayIndex axis) const override { return header_.spacing(axis); }
  Stride::Actual::value_type stride(const ArrayIndex axis) const override {
    return axis < parent().ndim() ? parent().stride(axis) : Stride::Actual::value_type(0);
  }

  VoxelIndex get_index(const ArrayIndex axis) const override { return pos_[axis]; }
  void move_index(const ArrayIndex axis, const VoxelIndex increment) override {
    pos_[axis] += increment;
    if (axis < parent().ndim())
      if (parent().size(axis) > 1)
        parent().index(axis) += increment;
  }

protected:
  using base_type::parent;
  Header header_;
  std::vector<VoxelIndex> pos_;
};

} // namespace MR::Adapter
