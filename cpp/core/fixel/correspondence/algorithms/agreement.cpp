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

#include "fixel/correspondence/algorithms/agreement.h"

#include <cmath>

#include "app.h"
#include "math/math.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

// clang-format off
OptionGroup AgreementOptions =
    OptionGroup("Options specific to algorithm \"agreement\"")
  + Option("agreement_kernel",
           "the angular cost kernel a(theta): \"tan\" or \"tan2\""
           " (default: tan2)")
    + Argument("choice").type_choice(angular_kernel_choices)
  + Option("agreement_sigma",
           "the density-contrast protection scale \"sigma\" beyond which density disagreement saturates"
           " (default: " + str(default_agreement_sigma) + ")")
    + Argument("value").type_float(0.0f)
  + Option("agreement_complexity",
           "weight \"beta\" applied to the squared parsimony penalty on fixel merging and splitting"
           " (default: " + str(default_agreement_complexity) + ")")
    + Argument("value").type_float(0.0f);
// clang-format on

AngularKernel Agreement::kernel = AngularKernel::TAN2;
float Agreement::s_scale = default_agreement_sigma;
float Agreement::b = default_agreement_complexity;

float Agreement::calculate(const std::vector<Correspondence::Fixel> &s,
                           const std::vector<Correspondence::Fixel> &rs,
                           const std::vector<Correspondence::Fixel> &t,
                           const std::vector<std::vector<index_type>> &inv_mapping,
                           const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());
  const float sigma2 = Math::pow2(s_scale);
  float result = 0.0f;

  // Per-template contributions
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
    const float d_t = t[t_index].density();
    const float d_rs = rs[t_index].density();
    if (d_rs > 0.0f) {
      // Angle-gated, saturating density disagreement
      const float dd = d_t - d_rs;
      const float a = angular_cost(std::min(1.0f, rs[t_index].absdot(t[t_index])), kernel);
      result += sigma2 * a * (1.0f - std::exp(-Math::pow2(dd) / sigma2));
    } else {
      // Unmatched template fixel
      result += Math::pow2(d_t);
    }
    // Parsimony: merging multiple subject fixels into one template fixel
    const int n_origins = static_cast<int>(origins_per_remapped_fixel[t_index]);
    if (n_origins > 1)
      result += b * Math::pow2(static_cast<float>(n_origins - 1));
  }

  // Per-subject contributions
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    const std::size_t n_objectives = inv_mapping[s_index].size();
    if (n_objectives == 0)
      result += Math::pow2(s[s_index].density()); // unmatched subject fixel
    else if (n_objectives > 1)
      result += b * Math::pow2(static_cast<float>(n_objectives - 1)); // splitting parsimony
  }

  return result;
}

void Agreement::set_constants(const AngularKernel kernel_in, const float sigma, const float complexity) {
  kernel = kernel_in;
  s_scale = sigma;
  b = complexity;
}

} // namespace MR::Fixel::Correspondence::Algorithms
