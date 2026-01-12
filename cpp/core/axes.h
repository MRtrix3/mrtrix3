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
#include <set>

#include "types.h"

// TODO Rename to "SpatialAxes"?
namespace MR::Axes {

// TODO Change to 8-bit integer & define invalid value
class permutations_type : public std::array<uint8_t, 3> {
public:
  using BaseType = std::array<uint8_t, 3>;
  permutations_type()
      : std::array<uint8_t, 3>{std::numeric_limits<uint8_t>::max(),
                               std::numeric_limits<uint8_t>::max(),
                               std::numeric_limits<uint8_t>::max()} {}
  bool is_identity() const { return ((*this)[0] == 0 && (*this)[1] == 1 && (*this)[2] == 2); }
  bool valid() const { return (std::set<ssize_t>(begin(), end()) == std::set<ssize_t>({0, 1, 2})); }
};
using flips_type = std::array<bool, 3>;
class Shuffle {
public:
  Shuffle() : permutations(), flips({false, false, false}) {}
  bool is_identity() const {
    return (permutations.is_identity() && //
            !flips[0] && !flips[1] && !flips[2]);
  }
  bool valid() const { return permutations.valid(); }
  permutations_type permutations;
  flips_type flips;
};

//! determine the axis permutations and flips necessary to make an image
//!   appear approximately axial
Shuffle get_shuffle_to_make_RAS(const transform_type &T);

//! determine which vectors of a 3x3 transform are closest to the three axis indices
permutations_type closest(const Eigen::Matrix3d &M);

} // namespace MR::Axes
