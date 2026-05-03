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

#include "fixel/correspondence/algorithms/in2023.h"

#include "app.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

// clang-format off
OptionGroup IN2023Options = OptionGroup("Options specific to algorithm \"in2023\"")
  + Option("in2023_constants",
           "set values for the two constants"
           " that modulate the influence of different cost function terms in the IN2023 expression")
    + Argument("alpha").type_float(0.0)
    + Argument("beta").type_float(0.0);
// clang-format off

float IN2023::a = default_in2023_alpha;
float IN2023::b = default_in2023_beta;

float IN2023::calculate(const std::vector<Correspondence::Fixel> &s,
                        const std::vector<Correspondence::Fixel> &rs,
                        const std::vector<Correspondence::Fixel> &t,
                        const std::vector<std::vector<index_type>> &inv_mapping,
                        const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());
  float result = 0.0f;
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {

    result += t[t_index].density() * (rs[t_index].density() ? dp2cost(t[t_index].absdot(rs[t_index])) : 1.0f);

    result += a * Math::pow2(t[t_index].density() - rs[t_index].density());

    result += b * Math::pow2(origins_per_remapped_fixel[t_index] - int8_t(1));
  }

  for (index_type s_index = 0; s_index != s.size(); ++s_index) {

    if (inv_mapping[s_index].empty()) {
      result += s[s_index].density();
      result += a * Math::pow2(s[s_index].density());
    }

    result += b * Math::pow2(static_cast<int8_t>(inv_mapping[s_index].size()) - int8_t(1));
  }

  return result;
}

void IN2023::set_constants(const float alpha, const float beta) {
  a = alpha;
  b = beta;
}

} // namespace MR::Fixel::Correspondence::Algorithms
