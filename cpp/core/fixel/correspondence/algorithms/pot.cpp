/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "fixel/correspondence/algorithms/pot.h"

#include <cmath>

#include "app.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

OptionGroup POTOptions =
    OptionGroup("Options specific to algorithm \"pot\"") +
    Option("pot_steepness",
           "exponent \"p\" controlling the angular sensitivity of the directional misalignment cost"
           " (default: " +
               str(default_pot_p) + ")") +
    Argument("value").type_float(1.0) +
    Option("pot_complexity",
           "weight \"gamma\" applied to the linear penalty for merging multiple subject fixels into one template fixel"
           " or splitting one subject fixel across multiple template fixels"
           " (default: " +
               str(default_pot_gamma) + ")") +
    Argument("value").type_float(0.0);

float POT::p = default_pot_p;
float POT::g = default_pot_gamma;

float POT::calculate(const std::vector<Correspondence::Fixel> &s,
                     const std::vector<Correspondence::Fixel> &rs,
                     const std::vector<Correspondence::Fixel> &t,
                     const std::vector<std::vector<index_type>> &inv_mapping,
                     const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());
  float result = 0.0f;

  // Per-pair (remapped <-> template) contributions
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
    const float d_t = t[t_index].density();
    const float d_rs = rs[t_index].density();

    // Matched mass undergoes a bounded directional misalignment cost.
    // Skip evaluating the dot product when no subject fixel was assigned;
    //   the remapped direction is undefined in that case
    //   and "min(d_t, 0) = 0" zeros the angular term anyway
    const float matched_mass = std::min(d_t, d_rs);
    if (matched_mass > 0.0f) {
      const float dp = t[t_index].absdot(rs[t_index]);
      const float aligned_fraction = (p == 1.0f) ? dp : std::pow(dp, p);
      result += 2.0f * matched_mass * (1.0f - aligned_fraction);
    }

    // Surplus mass on either side is created/destroyed at unit cost;
    //   this also covers unmatched template fixels (d_rs = 0 -> result += d_t)
    result += std::fabs(d_t - d_rs);

    // Linear penalty per "extra" subject fixel merged into this remapped fixel,
    //   weighted by template density to keep units consistent
    const int n_origins = static_cast<int>(origins_per_remapped_fixel[t_index]);
    if (n_origins > 1)
      result += g * d_t * static_cast<float>(n_origins - 1);
  }

  // Per-subject contributions: account for subject fixels with no objective,
  //   and for subject fixels that have been split across multiple template fixels
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    const float d_s = s[s_index].density();
    const std::size_t n_objectives = inv_mapping[s_index].size();

    if (n_objectives == 0) {
      result += d_s;
    } else if (n_objectives > 1) {
      result += g * d_s * static_cast<float>(n_objectives - 1);
    }
  }

  return result;
}

void POT::set_constants(const float p_in, const float gamma_in) {
  p = p_in;
  g = gamma_in;
}

} // namespace MR::Fixel::Correspondence::Algorithms
