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

#include <tcb/span.hpp>
#include <cstdint>
#include <limits>
#include <vector>

namespace MR {

struct ConvergenceChecker {

  struct Config {
    // Minimum required cost improvement to reset patience counter
    uint32_t patienceLimit = 10;
    // Absolute thresholds for each parameter
    // NOTE: the order must match the order of parameters in the optimization
    std::vector<float> param_thresholds;
  };

  explicit ConvergenceChecker(const Config &checkerConfiguration);

  bool has_converged(tcb::span<const float> current_transform_params, float current_cost);

  void reset_patience();

private:
  float m_minimum_cost = std::numeric_limits<float>::max();
  uint32_t m_patience_counter = 0;
  bool m_initialized = false;
  Config m_configuration;
  std::vector<float> m_best_params;
};

} // namespace MR
