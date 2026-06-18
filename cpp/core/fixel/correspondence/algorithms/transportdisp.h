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

extern App::OptionGroup TransportDispOptions;

/// @brief Source-mass transport variant scoring the merged remapped fixel.
///
/// As with Transport, template fibre density enters only through direction.
///   Where Transport pays a per-subject angular cost (so each off-axis fixel
///   pays its own way), this variant instead transports the assembled mass of
///   each remapped fixel at the alignment cost of its mean direction, plus an
///   explicit within-fixel dispersion penalty proportional to (1 - R), where R
///   is the density-weighted mean resultant length of the merged subject fixels.
///
/// This more strongly rewards merging fixels that straddle a template direction
///   (their mean aligns), while the dispersion term prevents the conflation of
///   widely-separated populations. Single-origin remapped fixels have R = 1 and
///   so incur no dispersion penalty.
class TransportDisp : public Combinatorial<TransportDisp> {
public:
  TransportDisp(const index_type max_origins_per_target,
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
  /// @param dispersion    within-fixel dispersion weight lambda
  static void set_constants(const AngularKernel kernel,
                            const float angle_degrees,
                            const float complexity,
                            const float dispersion);

protected:
  static AngularKernel kernel;
  static float cap; // angular cost a(theta*), i.e. unplaceable-mass penalty per unit density
  static float g;   // parsimony weight gamma
  static float l;   // dispersion weight lambda
};

} // namespace MR::Fixel::Correspondence::Algorithms
