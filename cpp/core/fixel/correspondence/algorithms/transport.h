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

extern App::OptionGroup TransportOptions;

/// @brief Source-mass transport correspondence cost function.
///
/// Each subject fixel carries its fibre density as "mass", split equally among
///   the template fixels to which it is assigned, and transported to each at an
///   angular cost a(theta) weighted by the transported mass. Subject mass that
///   cannot be placed within the threshold angle theta* is penalised at the
///   angular cost a(theta*) per unit density, making it preferable to leave a
///   subject fixel unmatched than to transport it beyond theta*.
///
/// Template fibre density appears only through template direction; it is never
///   a matching target. The optimal topology is therefore (near-)invariant to
///   the subject's density magnitudes, so between-subject density contrast is
///   preserved by construction rather than shrunk toward the template.
///
/// A parsimony penalty linear in the number of excess merges/splits, weighted
///   by source-derived density, discourages unnecessarily complex mappings.
class Transport : public Combinatorial<Transport> {
public:
  Transport(const index_type max_origins_per_target, const index_type max_objectives_per_source, const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel);

  /// @brief Shared per-source transport evaluation, reused by TransportGuard.
  static float transport_core(const std::vector<Correspondence::Fixel> &s,
                              const std::vector<Correspondence::Fixel> &rs,
                              const std::vector<Correspondence::Fixel> &t,
                              const std::vector<std::vector<index_type>> &inv_mapping,
                              const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel,
                              const AngularKernel kernel,
                              const float a_cap,
                              const float complexity);

  /// @brief Configure from the command line.
  /// @param kernel        angular cost kernel
  /// @param angle_degrees threshold angle theta* (degrees)
  /// @param complexity    parsimony weight gamma
  static void set_constants(const AngularKernel kernel, const float angle_degrees, const float complexity);

protected:
  static AngularKernel kernel;
  static float cap; // angular cost a(theta*), i.e. unplaceable-mass penalty per unit density
  static float g;   // parsimony weight gamma
};

} // namespace MR::Fixel::Correspondence::Algorithms
