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
#include <vector>

// AdaBelief is an improved version of Adam that takes into account the curvature of the loss function.
// See here https://arxiv.org/abs/2010.07468
// This version is further enhanced by the idea in this paper https://arxiv.org/abs/2411.16085
// which consists in performing an element‐wise mask to the update such that only the components
// where the proposed update direction and the current gradient are aligned
// (i.e., have the same sign) are applied. This ensures that every step reliably reduces the loss
// and avoids potential overshooting or oscillations.
class AdaBelief {
public:
  struct Parameter {
    float value;
    float learning_rate;
  };

  AdaBelief(const std::vector<Parameter> &parameters, float beta1 = 0.7F, float beta2 = 0.9999F, float epsilon = 1e-6F);

  void step(const std::vector<float> &gradients);
  std::vector<float> parameterValues() const;
  void setParameterValues(tcb::span<const float> values);

  // Resets the optimizer internal state (moments and timestep) but keeps the parameters unchanged
  void reset();

private:
  std::vector<Parameter> m_parameters;
  float m_beta1;
  float m_beta2;
  float m_epsilon;
  int m_timeStep;
  std::vector<float> m_firstMoments;  // Exponential moving average of gradients (m_t)
  std::vector<float> m_secondMoments; // Exponential moving average of squared deviations ((g_t - m_t)^2)
  std::vector<uint32_t> m_mask;
  std::vector<float> m_updates;
};
