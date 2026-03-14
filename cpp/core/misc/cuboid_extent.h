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
#include <limits>
#include <ostream>

#include "debug.h"
#include "mrtrix.h"
#include "types.h"

namespace MR {

//! used to encode the spatial extent of a cuboid kernel
class CuboidExtent {
public:
  using value_type = VoxelIndex;
  CuboidExtent() : data{0, 0, 0} {}
  CuboidExtent(const value_type extent) : data{extent, extent, extent} { check(); }
  template <typename ValueType> CuboidExtent(const std::vector<ValueType> &in) {
    if (std::numeric_limits<ValueType>::is_signed && *std::min_element(in.begin(), in.end()) < 0)
      throw Exception("Cuboid kernel extent cannot be negative");
    switch (in.size()) {
    case 1:
      data = {static_cast<value_type>(in[0]), static_cast<value_type>(in[0]), static_cast<value_type>(in[0])};
      break;
    case 3:
      data = {static_cast<value_type>(in[0]), static_cast<value_type>(in[1]), static_cast<value_type>(in[2])};
      break;
    default:
      throw Exception(std::string("Cuboid kernel extent must be initialised using either one value or three;") +
                      "received \"" + str(in) + "\"");
    }
    check();
  }
  bool is_unity() const { return std::max({data[0], data[1], data[2]}) == 1; }
  size_t nvoxels() const {
    return static_cast<size_t>(data[0]) * static_cast<size_t>(data[1]) * static_cast<size_t>(data[2]);
  }
  operator bool() const { return std::min({data[0], data[1], data[2]}) > 0; }
  value_type operator[](const ArrayIndex &axis) const {
    assert(axis >= 0 && axis < 3);
    return data[axis];
  }
  friend std::ostream &operator<<(std::ostream &stream, const CuboidExtent &extent) {
    stream << extent[0] << "x" << extent[1] << "x" << extent[2];
    return stream;
  }

private:
  std::array<value_type, 3> data;
  void check() const {
    for (auto value : data) {
      if (value < 1)
        throw Exception("Cuboid kernel extent must be positive");
      if (value & value_type(1) == 0)
        throw Exception("Cuboid kernel extent must be odd");
    }
  }
};

} // namespace MR
