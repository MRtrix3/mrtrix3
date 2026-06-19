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

#include "fixel/correspondence/algorithms/transportdisp.h"

#include <cmath>

#include "app.h"
#include "math/math.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

// clang-format off
OptionGroup TransportDispOptions =
    OptionGroup("Options specific to algorithm \"transportdisp\""
                " (in addition to those shared with \"transport\")")
  + Option("transportdisp_dispersion",
           "weight \"lambda\" applied to the within-fixel angular dispersion penalty"
           " (default: " + str(default_transportdisp_dispersion) + ")")
    + Argument("value").type_float(0.0f);
// clang-format on

AngularKernel TransportDisp::kernel = AngularKernel::TAN2;
float TransportDisp::cap = 1.0f; // tan^2(45 degrees) = 1
float TransportDisp::g = default_transport_complexity;
float TransportDisp::l = default_transportdisp_dispersion;

float TransportDisp::calculate(const std::vector<Correspondence::Fixel> &s,
                               const std::vector<Correspondence::Fixel> &rs,
                               const std::vector<Correspondence::Fixel> &t,
                               const std::vector<std::vector<index_type>> &inv_mapping,
                               const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());

  // Reconstruct, for each template fixel, the set of subject fixels merged into it
  std::vector<std::vector<index_type>> origins(t.size());
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    for (const index_type t_index : inv_mapping[s_index])
      origins[t_index].push_back(s_index);
  }

  float result = 0.0f;

  // Per-template contributions: transport the assembled remapped mass at the
  //   alignment cost of its mean direction, plus a within-fixel dispersion penalty
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
    const float d_rs = rs[t_index].density();
    if (d_rs <= 0.0f)
      continue;

    const float align = angular_cost(std::min(1.0f, rs[t_index].absdot(t[t_index])), kernel);

    // Density-weighted mean resultant length R of the merged subject fixels,
    //   each sign-aligned to the template direction (as during remapping)
    dir_t resultant = dir_t::Zero();
    for (const index_type s_index : origins[t_index]) {
      const float mass = s[s_index].density() / static_cast<float>(inv_mapping[s_index].size());
      const float sign = (t[t_index].dot(s[s_index]) < 0.0f) ? -1.0f : 1.0f;
      resultant += mass * sign * s[s_index].dir();
    }
    const float R = resultant.norm() / d_rs; // in [0, 1]; equals 1 for a single origin

    result += d_rs * (align + l * (1.0f - R));

    // Parsimony: merging multiple subject fixels into one template fixel
    if (origins[t_index].size() > 1)
      result += g * d_rs * static_cast<float>(origins[t_index].size() - 1);
  }

  // Per-subject contributions: unplaced mass, and splitting parsimony
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    const float d_s = s[s_index].density();
    const std::size_t n_objectives = inv_mapping[s_index].size();
    if (n_objectives == 0)
      result += d_s * cap;
    else if (n_objectives > 1)
      result += g * d_s * static_cast<float>(n_objectives - 1);
  }

  return result;
}

void TransportDisp::set_constants(const AngularKernel kernel_in,
                                  const float angle_degrees,
                                  const float complexity,
                                  const float dispersion) {
  kernel = kernel_in;
  const float cos_theta_star = static_cast<float>(std::cos(angle_degrees * Math::pi / 180.0));
  cap = angular_cost(cos_theta_star, kernel_in);
  g = complexity;
  l = dispersion;
}

} // namespace MR::Fixel::Correspondence::Algorithms
