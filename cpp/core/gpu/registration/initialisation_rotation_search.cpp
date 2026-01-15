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

#include "gpu/registration/initialisation_rotation_search.h"
#include "gpu/registration/registrationtypes.h"
#include <tcb/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <numeric>
#include <vector>

namespace MR::GPU {

Eigen::Vector3f search_best_rotation(const GlobalTransform &initial_transform,
                                     tcb::span<const std::array<float, 3>> samples,
                                     const std::function<RotationSearchCalculator()> &make_calculator,
                                     const RotationSearchParams &params,
                                     const std::function<void(float, const std::array<float, 3> &)> &on_update) {
  if (params.parallel_calculators == 0 || samples.empty()) {
    return Eigen::Vector3f::Zero();
  }

  const auto rotation_angle = [](const std::array<float, 3> &axis) {
    return std::sqrt(std::inner_product(axis.cbegin(), axis.cend(), axis.cbegin(), 0.0F));
  };

  float best_cost = std::numeric_limits<float>::infinity();
  std::array<float, 3> best_rotation = {0.0F, 0.0F, 0.0F};

  const auto consider_candidate = [&](float candidate_cost, const std::array<float, 3> &sample) {
    const float cost_delta = candidate_cost - best_cost;
    const bool better_cost = cost_delta < -params.min_improvement;
    const bool tie_with_smaller_angle =
        std::abs(cost_delta) <= params.tie_cost_eps && rotation_angle(sample) < rotation_angle(best_rotation);

    if (better_cost || tie_with_smaller_angle) {
      best_cost = candidate_cost;
      best_rotation = sample;
      if (on_update) {
        on_update(best_cost, best_rotation);
      }
    }
  };

  std::vector<RotationSearchCalculator> calculators;
  calculators.reserve(params.parallel_calculators);
  for (size_t i = 0; i < params.parallel_calculators; ++i) {
    calculators.push_back(make_calculator());
  }

  const size_t param_count = initial_transform.param_count();
  std::array<float, 12> base_params{};
  const auto initial_params = initial_transform.parameters();
  std::copy(initial_params.begin(), initial_params.end(), base_params.begin());

  for (size_t chunk_start = 0; chunk_start < samples.size(); chunk_start += calculators.size()) {
    const size_t chunk_size = std::min(calculators.size(), samples.size() - chunk_start);

    for (size_t local_index = 0; local_index < chunk_size; ++local_index) {
      const auto &sample = samples[chunk_start + local_index];
      auto params = base_params;
      params[3] = sample[0];
      params[4] = sample[1];
      params[5] = sample[2];
      const GlobalTransform candidate_transform(
          tcb::span<const float>(params.data(), param_count), initial_transform.type(), initial_transform.pivot());
      calculators[local_index].update(candidate_transform);
    }

    for (size_t local_index = 0; local_index < chunk_size; ++local_index) {
      const auto result = calculators[local_index].get_result();
      consider_candidate(result.cost, samples[chunk_start + local_index]);
    }
  }

  return Eigen::Vector3f(best_rotation[0], best_rotation[1], best_rotation[2]);
}

Eigen::Vector3f search_best_rotation(const GlobalTransform &initial_transform,
                                     tcb::span<const std::array<float, 3>> samples,
                                     const std::function<RotationSearchCalculator()> &make_calculator,
                                     const RotationSearchParams &params) {
  return search_best_rotation(
      initial_transform, samples, make_calculator, params, std::function<void(float, const std::array<float, 3> &)>());
}

} // namespace MR::GPU
