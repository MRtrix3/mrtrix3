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

extern App::OptionGroup AgreementOptions;

/// @brief Angle-gated, saturating density-agreement cost function.
///
/// A conservative evolution of the ISMRM2018 cost. As there, the density
///   disagreement between each remapped and template fixel is gated by angular
///   misalignment a(theta), so that where the geometry is good the cost is
///   insensitive to density and between-subject contrast is preserved.
///
/// The unbounded (d_t - d_rs)^2 term of ISMRM2018 is replaced by a saturating
///   form sigma^2 * (1 - exp(-(d_t - d_rs)^2 / sigma^2)): near zero it matches
///   the original quadratic, but it saturates at sigma^2 once the disagreement
///   is clearly non-zero, so a subject whose density genuinely differs from the
///   template is not dragged back toward it. A squared parsimony penalty on
///   merges/splits (absent from ISMRM2018) stabilises the chosen topology.
class Agreement : public Combinatorial<Agreement> {
public:
  Agreement(const index_type max_origins_per_target, const index_type max_objectives_per_source, const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel);

  /// @brief Configure from the command line.
  /// @param kernel     angular cost kernel
  /// @param sigma      density-contrast protection scale (density units)
  /// @param complexity squared-parsimony weight beta
  static void set_constants(const AngularKernel kernel, const float sigma, const float complexity);

protected:
  static AngularKernel kernel;
  static float s_scale; // contrast-protection scale sigma
  static float b;       // squared-parsimony weight beta
};

} // namespace MR::Fixel::Correspondence::Algorithms
