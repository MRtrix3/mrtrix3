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

namespace MR::Adapter {

template <class ImageType> class Extract1D : public Base<Extract1D<ImageType>, ImageType> {
public:
  using base_type = Base<Extract1D<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::ndim;
  using base_type::parent;
  using base_type::spacing;

  Extract1D(const ImageType &original, const Eigen::Index axis, const std::vector<Axes::index_type> &indices)
      : base_type(original), extract_axis(axis), indices(indices), nsize(indices.size()), trans(original.transform()) {
    assert(*std::min_element(indices.begin(), indices.end()) >= 0);
    assert(*std::max_element(indices.begin(), indices.end()) < original.size(axis));

    reset();

    if (extract_axis < 3) {
      Eigen::Vector3d a(0.0, 0.0, 0.0);
      a[extract_axis] = indices[0] * spacing(extract_axis);
      trans.translation() = trans * a;
    }

    this->indices.push_back(indices.back());
  }

  void reset() {
    for (Eigen::Index axis = 0; axis < ndim(); ++axis)
      parent().index(axis) = (axis == extract_axis ? indices[0] : 0);
    current_pos = 0;
  }

  Eigen::Index size(const Eigen::Index axis) const { return (axis == extract_axis ? nsize : base_type::size(axis)); }

  const transform_type &transform() const { return trans; }

  Axes::index_type get_index(const Eigen::Index axis) const {
    return (axis == extract_axis ? current_pos : parent().index(axis));
  }
  void move_index(const Eigen::Index axis, const Axes::index_type increment) {
    // VAR(extract_axis);
    // VAR(indices);
    // VAR(nsize);
    // VAR(current_pos);
    // VAR(axis);
    // VAR(increment);
    if (axis == extract_axis) {
      const Axes::index_type prev_pos = current_pos < nsize ? indices[current_pos] : 0;
      current_pos += increment;
      if (current_pos < nsize)
        parent().index(axis) += indices[current_pos] - prev_pos;
      else
        parent().index(axis) = 0;
    } else {
      parent().index(axis) += increment;
    }
    // VAR(current_pos);
    // VAR(parent().index(axis));
  }

  friend std::ostream &operator<<(std::ostream &stream, const Extract1D &V) {
    stream << "Extract1D adapter for image \"" << V.name() << "\", position [ ";
    for (Eigen::Index n = 0; n < V.ndim(); ++n)
      stream << V.index(n) << " ";
    stream << "], value = " << V.value();
    return stream;
  }

private:
  const Eigen::Index extract_axis;
  std::vector<Axes::index_type> indices;
  const Eigen::Index nsize;
  transform_type trans;
  Axes::index_type current_pos;
};

template <class ImageType> class Extract : public Base<Extract<ImageType>, ImageType> {
public:
  using base_type = Base<Extract<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::ndim;
  using base_type::parent;
  using base_type::spacing;

  Extract(const ImageType &original, const std::vector<std::vector<Axes::index_type>> &indices)
      : base_type(original), current_pos(ndim()), indices(indices), trans(original.transform()) {
    reset();
    trans.translation() = trans * Eigen::Vector3d(indices[0][0] * spacing(0),  //
                                                  indices[1][0] * spacing(1),  //
                                                  indices[2][0] * spacing(2)); //

    for (const auto &i : indices)
      sizes.push_back(i.size());
  }

  Eigen::Index size(const Eigen::Index axis) const { return sizes[axis]; }

  const transform_type &transform() const { return trans; }

  void reset() {
    for (Eigen::Index n = 0; n < ndim(); ++n) {
      current_pos[n] = 0;
      parent().index(n) = indices[n][0];
    }
  }

  Axes::index_type get_index(const Eigen::Index axis) const { return current_pos[axis]; }
  void move_index(const Eigen::Index axis, const Axes::index_type increment) {
    current_pos[axis] += increment;
    if (current_pos[axis] < 0)
      parent().index(axis) = -1;
    else if (current_pos[axis] >= sizes[axis])
      parent().index(axis) = parent().size(axis);
    else
      parent().index(axis) = indices[axis][current_pos[axis]];
  }

private:
  std::vector<Axes::index_type> current_pos;
  std::vector<std::vector<Axes::index_type>> indices;
  std::vector<Eigen::Index> sizes;
  transform_type trans;
};

} // namespace MR::Adapter
