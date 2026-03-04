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

#include "algo/iterator.h"
#include "axes.h"
#include "types.h"

namespace MR {

/** \defgroup loop Looping functions
  @{ */

//! a dummy image to iterate over a certain neighbourhood, useful for multi-threaded looping.
// Does not work properly with Loop() functions! Use instead:
//
// vector extent(iter.ndim(),3) // max number of voxels to iterate over
// auto niter = NeighbourhoodIterator(iter, extent);
// while(niter.loop()){
//   std::cerr << niter << std::endl;
// }
//
//
class NeighbourhoodIterator {
public:
  NeighbourhoodIterator() = delete;
  template <class IteratorType>
  NeighbourhoodIterator(const IteratorType &iter, const std::vector<Eigen::Index> &extent)
      : dim(iter.ndim()),
        offset(iter.ndim()),
        // pos (iter.ndim()),
        pos_orig(iter.ndim()),
        ext(container_cast<decltype(ext)>(extent)),
        has_next_(false) {
    assert(iter.ndim() == extent.size());
    pos.resize(iter.ndim());
    for (size_t i = 0; i < iter.ndim(); ++i) {
      ext[i] = (ext[i] - 1) / 2;
      offset[i] = iter.index(i);
      // set pos to lower bound
      pos[i] = (offset[i] - ext[i] < 0) ? 0 : offset[i] - ext[i];
      pos_orig[i] = pos[i];
      // upper bound:
      auto high = (offset[i] + ext[i] >= iter.size(i)) ? iter.size(i) - 1 : offset[i] + ext[i];
      dim[i] = high - pos[i] + 1;
    }
  }

  Eigen::Index ndim() const { return dim.size(); }
  Eigen::Index size(const Eigen::Index axis) const { return dim[axis]; }

  const Axes::index_type &index(const Eigen::Index axis) const { return pos[axis]; }
  Axes::index_type &index(const Eigen::Index axis) { return pos[axis]; }

  const Eigen::Matrix<Axes::index_type, 1, Eigen::Dynamic> get_pos() const { return pos; }

  const Eigen::Index &extent(const Eigen::Index axis) const { return dim[axis]; }
  const Axes::index_type &centre(const Eigen::Index axis) const { return offset[axis]; }

  void reset(const Eigen::Index axis) { pos[axis] = pos_orig[axis]; }

  bool loop() {
    if (not this->has_next_) {
      this->has_next_ = true;
      for (auto axis = dim.size(); axis-- != 0;)
        this->reset(axis);
      return true;
    }
    for (auto axis = dim.size(); axis-- != 0;) { // TODO loop in stride order
      ++pos[axis];
      if (pos[axis] != (pos_orig[axis] + dim[axis]))
        return true;
      this->reset(axis);
    }
    this->has_next_ = false;
    return false;
  }

  friend std::ostream &operator<<(std::ostream &stream, const NeighbourhoodIterator &V) {
    stream << "neighbourhood iterator, position [ ";
    for (size_t n = 0; n < V.ndim(); ++n)
      stream << V.index(n) << " ";
    stream << "], extent [ ";
    for (size_t n = 0; n < V.ndim(); ++n)
      stream << V.extent(n) << " ";
    stream << "], centre [ ";
    for (size_t n = 0; n < V.ndim(); ++n)
      stream << V.centre(n) << " ";
    stream << "]";
    return stream;
  }

private:
  std::vector<Eigen::Index> dim;
  std::vector<Axes::index_type> offset;
  std::vector<Axes::index_type> pos_orig;
  std::vector<Eigen::Index> ext;
  Eigen::Matrix<Axes::index_type, 1, Eigen::Dynamic> pos;
  bool has_next_;

  void value() const { assert(0); }
};

//! @}
} // namespace MR
