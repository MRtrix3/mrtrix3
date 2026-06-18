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

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace MR::Fixel::Correspondence::Algorithms {

/// @brief Selectable angular cost kernel a(theta) for the proposed cost functions.
///
/// Both kernels are zero at theta=0, increase monotonically with angle,
///   and diverge as theta approaches 90 degrees;
///   they cross unity at 45 degrees and differ only in steepness.
enum class AngularKernel { TAN, TAN2 };

/// @brief Command-line choice strings; index 0 -> TAN, index 1 -> TAN2.
inline const std::vector<std::string> angular_kernel_choices{"tan", "tan2"};

/// @brief Angular cost a(theta) evaluated from |cos(theta)|.
///
/// @param abs_dp  the absolute dot product |cos(theta)|, in [0, 1]
/// @param kernel  TAN -> tan(theta); TAN2 -> tan^2(theta) = (1 - c^2) / c^2
///
/// Orthogonal directions (abs_dp == 0) return +inf, forbidding the assignment.
inline float angular_cost(const float abs_dp, const AngularKernel kernel) {
  const float c2 = abs_dp * abs_dp;
  if (c2 <= 0.0f)
    return std::numeric_limits<float>::infinity();
  const float tan2 = (1.0f - c2) / c2;
  return kernel == AngularKernel::TAN2 ? tan2 : std::sqrt(tan2);
}

} // namespace MR::Fixel::Correspondence::Algorithms
