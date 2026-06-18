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

#include "fixel/correspondence/algorithms/transport.h"

#include <cmath>

#include "app.h"
#include "math/math.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

// clang-format off
OptionGroup TransportOptions =
    OptionGroup("Options specific to algorithms \"transport\", \"transportdisp\" and \"transportguard\"")
  + Option("transport_kernel",
           "the angular cost kernel a(theta): \"tan\" or \"tan2\""
           " (default: tan2)")
    + Argument("choice").type_choice(angular_kernel_choices)
  + Option("transport_angle",
           "the threshold angle theta* (degrees) beyond which subject fibre density is left unplaced"
           " rather than transported to a poorly-aligned template fixel"
           " (default: " + str(default_transport_angle) + ")")
    + Argument("value").type_float(0.0f, 90.0f)
  + Option("transport_complexity",
           "weight \"gamma\" applied to the linear parsimony penalty on fixel merging and splitting"
           " (default: " + str(default_transport_complexity) + ")")
    + Argument("value").type_float(0.0f);
// clang-format on

AngularKernel Transport::kernel = AngularKernel::TAN2;
float Transport::cap = 1.0f; // tan^2(45 degrees) = 1
float Transport::g = default_transport_complexity;

float Transport::transport_core(const std::vector<Correspondence::Fixel> &s,
                                const std::vector<Correspondence::Fixel> &rs,
                                const std::vector<Correspondence::Fixel> &t,
                                const std::vector<std::vector<index_type>> &inv_mapping,
                                const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel,
                                const AngularKernel kernel,
                                const float a_cap,
                                const float complexity) {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());
  float result = 0.0f;

  // Per-subject contributions: transport each subject fixel's mass to the
  //   template fixel(s) it serves, or penalise it if it is left unplaced
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    const float d_s = s[s_index].density();
    const std::size_t n_objectives = inv_mapping[s_index].size();

    if (n_objectives == 0) {
      // No corresponding template fixel within the threshold angle
      result += d_s * a_cap;
    } else {
      // Mass is split equally among the objective template fixels;
      //   each portion is transported at its own angular cost
      const float mass = d_s / static_cast<float>(n_objectives);
      for (const index_type t_index : inv_mapping[s_index])
        result += mass * angular_cost(s[s_index].absdot(t[t_index]), kernel);
      // Parsimony: splitting one subject fixel across multiple template fixels
      if (n_objectives > 1)
        result += complexity * d_s * static_cast<float>(n_objectives - 1);
    }
  }

  // Per-template parsimony: merging multiple subject fixels into one template fixel,
  //   weighted by the (source-derived) remapped density to keep units consistent
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
    const int n_origins = static_cast<int>(origins_per_remapped_fixel[t_index]);
    if (n_origins > 1)
      result += complexity * rs[t_index].density() * static_cast<float>(n_origins - 1);
  }

  return result;
}

float Transport::calculate(const std::vector<Correspondence::Fixel> &s,
                           const std::vector<Correspondence::Fixel> &rs,
                           const std::vector<Correspondence::Fixel> &t,
                           const std::vector<std::vector<index_type>> &inv_mapping,
                           const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  return transport_core(s, rs, t, inv_mapping, origins_per_remapped_fixel, kernel, cap, g);
}

void Transport::set_constants(const AngularKernel kernel_in, const float angle_degrees, const float complexity) {
  kernel = kernel_in;
  const float cos_theta_star = static_cast<float>(std::cos(angle_degrees * Math::pi / 180.0));
  cap = angular_cost(cos_theta_star, kernel_in);
  g = complexity;
}

} // namespace MR::Fixel::Correspondence::Algorithms
