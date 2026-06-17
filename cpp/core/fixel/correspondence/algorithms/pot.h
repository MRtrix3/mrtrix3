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

namespace MR::App {
class OptionGroup;
} // namespace MR::App

namespace MR::Fixel::Correspondence::Algorithms {

extern App::OptionGroup POTOptions;

/// @brief Partial-optimal-transport correspondence cost function.
///
/// For each remapped/template fixel pair the cost is
///   2 * min(d_t, d_rs) * (1 - |cos theta|^p)  +  |d_t - d_rs|
///   + gamma * d_t * max(0, n_origins - 1)
/// matched mass is "transported" at a directional cost,
///   surplus mass on either side is created or destroyed at unit cost,
///   and combining multiple subject fixels into one remapped fixel
///   incurs a penalty linear in the number of excess origins.
/// Each subject fixel further contributes
///   gamma * d_s * max(0, |inv_mapping| - 1)
///   for any splitting onto multiple template fixels;
///   subject fixels with no objective contribute their full density.
class POT : public Combinatorial<POT> {
public:
  POT(const index_type max_origins_per_target, const index_type max_objectives_per_source, const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel);

  // static void set_constants(const float p, const float gamma);
  static void set_gamma(const float gamma);

protected:
  // static float p;
  static float g;
};

} // namespace MR::Fixel::Correspondence::Algorithms
