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

#include "gpu/registration/registrationtypes.h"
#include <tcb/span.hpp>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <functional>

namespace MR::GPU {

struct RotationSearchParams {
  size_t parallel_calculators = 8;
  float min_improvement = 1e-6F;
  float tie_cost_eps = 1e-6F;
};

struct RotationSearchCalculator {
  std::function<void(const GlobalTransform &)> update;
  std::function<IterationResult()> get_result;
};

Eigen::Vector3f search_best_rotation(const GlobalTransform &initial_transform,
                                     tcb::span<const std::array<float, 3>> samples,
                                     const std::function<RotationSearchCalculator()> &make_calculator,
                                     const RotationSearchParams &params,
                                     const std::function<void(float, const std::array<float, 3> &)> &on_update);

Eigen::Vector3f search_best_rotation(const GlobalTransform &initial_transform,
                                     tcb::span<const std::array<float, 3>> samples,
                                     const std::function<RotationSearchCalculator()> &make_calculator,
                                     const RotationSearchParams &params);

} // namespace MR::GPU
