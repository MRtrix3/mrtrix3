/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <stdint.h>

#include "header.h"

#include "fixel/correspondence/algorithms/combinatorial.h"

namespace MR::App {
class OptionGroup;
} // namespace MR::App

namespace MR::Fixel::Correspondence::Algorithms {

extern App::OptionGroup IN2023Options;

class IN2023 : public Combinatorial<IN2023> {
public:
  IN2023(const index_type max_origins_per_target, const index_type max_objectives_per_source, const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel);

  static void set_constants(const float alpha, const float beta);

protected:
  static float a, b;
};

} // namespace MR::Fixel::Correspondence::Algorithms
