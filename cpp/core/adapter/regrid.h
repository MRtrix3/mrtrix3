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

template <class ImageType> class Regrid : public Base<Regrid<ImageType>, ImageType> {
public:
  using base_type = Base<Regrid<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::name;
  using base_type::spacing;

  template <class VectorType>
  Regrid(const ImageType &original, const VectorType &from, const VectorType &size, const value_type fill = 0)
      : base_type(original),
        from_(container_cast<decltype(from_)>(from)),
        size_(container_cast<decltype(size_)>(size)),
        index_invalid_lower_upper([&] {
          std::vector<std::vector<VoxelIndex>> v;
          for (size_t d = 0; d < from_.size(); ++d) {
            v.push_back(std::vector<VoxelIndex>{from_[d] < 0 ? -from_[d] - 1 : -1, original.size(d) - from_[d]});
          }
          return v;
        }()),
        index_requires_bound_check([&] {
          Eigen::Array<bool, Eigen::Dynamic, 1> v(Eigen::Array<bool, Eigen::Dynamic, 1>::Zero(from_.size()));
          for (ArrayIndex d = 0; d < from_.size(); ++d)
            v[d] = from_[d] < 0 || size_[d] > original.size(d) - from_[d];
          return v;
        }()),
        fill_(fill),
        transform_(original.transform()),
        index_(ndim(), 0) {
    assert(from_.size() == size_.size());
    assert(from_.size() == ndim());

    for (StdIndex n = 0; n < ndim(); ++n)
      if (size_[n] < 0)
        throw Exception("FIXME: negative size in Regrid adapter");

    // adjust location of origin
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

  VoxelIndex get_index(const ArrayIndex axis) const override {
    return index_requires_bound_check[axis] ? index_[axis] : parent().index(axis) - from_[axis];
  }

  void move_index(const ArrayIndex axis, const VoxelIndex increment) override {
    if (index_requires_bound_check[axis]) {
      index_[axis] += increment;
      if (increment > 0) {
        if (index_[axis] < index_invalid_lower_upper[axis][1])
          parent().index(axis) = index_[axis] + from_[axis];
      } else if (index_[axis] > index_invalid_lower_upper[axis][0])
        parent().index(axis) = index_[axis] + from_[axis];
    } else
      parent().index(axis) += increment;
  }

  value_type value() {
    for (StdIndex axis = 0; axis < index_.size(); ++axis)
      if (index_requires_bound_check[axis] &&
          (index_[axis] >= index_invalid_lower_upper[axis][1] || index_[axis] <= index_invalid_lower_upper[axis][0]))
        return fill_;
    return parent().value();
  }

protected:
  using base_type::parent;
  const std::vector<VoxelIndex> from_, size_;
  const std::vector<std::vector<VoxelIndex>> index_invalid_lower_upper;
  const Eigen::Array<bool, Eigen::Dynamic, 1> index_requires_bound_check;
  const value_type fill_;
  transform_type transform_;
  std::vector<VoxelIndex> index_;
};

} // namespace MR::Adapter
