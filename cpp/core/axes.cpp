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

#include "axes.h"

namespace MR::Axes {

Shuffle get_shuffle_to_make_RAS(const transform_type &T) {
  Shuffle result;
  result.permutations = closest(T.linear());
  // Figure out whether any of the rows of the transform point in the
  //   opposite direction to the MRtrix convention
  result.flips[result.permutations[0]] = T(0, result.permutations[0]) < 0.0;
  result.flips[result.permutations[1]] = T(1, result.permutations[1]) < 0.0;
  result.flips[result.permutations[2]] = T(2, result.permutations[2]) < 0.0;
  return result;
}

permutations_type closest(const Eigen::Matrix3d &M) {
  permutations_type result;
  // Find which row of the transform is closest to each scanner axis
  Eigen::Matrix3d::Index index(0);
  M.row(0).cwiseAbs().maxCoeff(&index);
  result[0] = index;
  M.row(1).cwiseAbs().maxCoeff(&index);
  result[1] = index;
  M.row(2).cwiseAbs().maxCoeff(&index);
  result[2] = index;
  assert(result.valid());

  // Disambiguate permutations
  auto not_any_of = [](size_t a, size_t b) -> size_t {
    for (size_t i = 0; i < 3; ++i) {
      if (a == i || b == i)
        continue;
      return i;
    }
    assert(0);
    return std::numeric_limits<size_t>::max();
  };
  if (result[0] == result[1])
    result[1] = not_any_of(result[0], result[2]);
  if (result[0] == result[2])
    result[2] = not_any_of(result[0], result[1]);
  if (result[1] == result[2])
    result[2] = not_any_of(result[0], result[1]);
  assert(result.valid());

  return result;
}

} // namespace MR::Axes
