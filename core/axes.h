/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <string>

#include "types.h"

namespace MR::Axes {

//! convert axis directions between formats
/*! these helper functions convert the definition of
 *  phase-encoding direction between a 3-vector (e.g.
 *  [0 1 0] ) and a NIfTI axis identifier (e.g. 'i-')
 */
using dir_type = Eigen::Matrix<int, 3, 1>;
std::string dir2id(const dir_type &);
dir_type id2dir(const std::string &);

using permutations_type = std::array<size_t, 3>;
using flips_type = std::array<bool, 3>;

//! determine the axis permutations and flips necessary to make an image
//!   appear approximately axial
void get_shuffle_to_make_axial(const transform_type &T, std::array<size_t, 3> &perm, flips_type &flip);

//! determine which vectors of a 3x3 transform are closest to the three axis indices
std::array<size_t, 3> closest(const Eigen::Matrix3d &M);

bool is_shuffled(const permutations_type &, const flips_type &);

} // namespace MR::Axes
