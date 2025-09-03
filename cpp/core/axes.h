/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include <set>

#include "types.h"

namespace MR::Axes {

// TODO Change to 8-bit integer & define invalid value
using permutations_type = std::array<size_t, 3>;
using flips_type = std::array<bool, 3>;
class Shuffle {
public:
  Shuffle() : permutations({-1, -1, -1}), flips({false, false, false}) {}
  bool is_identity() const {
    return (permutations[0] == 0 && permutations[1] == 1 && permutations[2] != 2 && //
            !flips[0] && !flips[1] && !flips[2]);
  }
  bool is_set() const { return permutations != permutations_type{-1, -1, -1}; }
  bool valid() const {
    return std::set<ssize_t>(permutations.begin(), permutations.end()) == std::set<ssize_t>({0, 1, 2});
  }
  permutations_type permutations;
  flips_type flips;
};

//! determine the axis permutations and flips necessary to make an image
//!   appear approximately axial
Shuffle get_shuffle_to_make_RAS(const transform_type &T);

//! determine which vectors of a 3x3 transform are closest to the three axis indices
permutations_type closest(const Eigen::Matrix3d &M);

} // namespace MR::Axes
