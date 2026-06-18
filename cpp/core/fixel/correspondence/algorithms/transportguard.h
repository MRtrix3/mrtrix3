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

#include "header.h"

#include "fixel/correspondence/algorithms/combinatorial.h"
#include "fixel/correspondence/algorithms/kernel.h"

namespace MR::App {
class OptionGroup;
} // namespace MR::App

namespace MR::Fixel::Correspondence::Algorithms {

extern App::OptionGroup TransportGuardOptions;

/// @brief Source-mass transport with a one-sided over-explanation guard.
///
/// Identical to Transport (template density enters only through direction), but
///   with an additional penalty mu * max(0, d_rs - rho * d_t)^2 that fires only
///   when a remapped fixel accumulates substantially more mass than the
///   template plausibly holds (rho > 1). The guard is exactly zero for any
///   d_rs <= rho * d_t, so ordinary between-subject contrast is untouched; it
///   only suppresses non-physical pile-ups from over-merging.
class TransportGuard : public Combinatorial<TransportGuard> {
public:
  TransportGuard(const index_type max_origins_per_target,
                 const index_type max_objectives_per_source,
                 const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel);

  /// @brief Configure from the command line.
  /// @param kernel        angular cost kernel
  /// @param angle_degrees threshold angle theta* (degrees)
  /// @param complexity    parsimony weight gamma
  /// @param mu            over-explanation penalty weight
  /// @param rho           over-explanation density ratio threshold (> 1)
  static void set_constants(const AngularKernel kernel,
                            const float angle_degrees,
                            const float complexity,
                            const float mu,
                            const float rho);

protected:
  static AngularKernel kernel;
  static float cap; // angular cost a(theta*), i.e. unplaceable-mass penalty per unit density
  static float g;   // parsimony weight gamma
  static float m;   // over-explanation penalty weight mu
  static float r;   // over-explanation density ratio threshold rho
};

} // namespace MR::Fixel::Correspondence::Algorithms
