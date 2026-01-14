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

#include "gpu/registration/convergencechecker.h"
#include "exception.h"
#include <tcb/span.hpp>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace MR {

ConvergenceChecker::ConvergenceChecker(const Config &checkerConfiguration) : m_configuration(checkerConfiguration) {
  assert(!m_configuration.param_thresholds.empty());
  assert(m_configuration.patienceLimit > 0U);
}

bool ConvergenceChecker::has_converged(tcb::span<const float> current_params, float current_cost) {
  if (m_configuration.param_thresholds.size() != current_params.size()) {
    throw std::invalid_argument("ConvergenceChecker::has_converged: parameter threshold configuration mismatch.");
  }

  if (!m_initialized) {
    DEBUG("ConvergenceChecker: Initializing with first parameters and cost.");
    m_minimum_cost = current_cost;
    m_best_params.assign(current_params.begin(), current_params.end());
    m_initialized = true;
    return false;
  }

  const bool has_better_cost = current_cost < m_minimum_cost;

  const auto significant_param_improvement = [&]() {
    for (size_t idx = 0; idx < current_params.size(); ++idx) {
      const float param_diff = std::fabs(m_best_params[idx] - current_params[idx]);
      if (param_diff >= m_configuration.param_thresholds[idx]) {
        return true;
      }
    }
    return false;
  }();

  if (has_better_cost) {
    m_minimum_cost = current_cost;
    m_best_params.assign(current_params.begin(), current_params.end());
    // For the patience counter, only significant parameter improvements count
    m_patience_counter = significant_param_improvement ? 0U : m_patience_counter + 1U;
    DEBUG("ConvergenceChecker: Better cost found. Resetting patience counter to " + std::to_string(m_patience_counter) +
          ".");
  } else {
    DEBUG("ConvergenceChecker: No better cost found. Incrementing patience counter.");
    ++m_patience_counter;
  }

  return m_patience_counter >= m_configuration.patienceLimit;
}

void ConvergenceChecker::reset_patience() { m_patience_counter = 0; }

} // namespace MR
