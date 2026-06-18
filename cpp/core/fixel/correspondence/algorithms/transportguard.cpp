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

#include "fixel/correspondence/algorithms/transportguard.h"

#include <cmath>

#include "app.h"
#include "math/math.h"

#include "fixel/correspondence/algorithms/transport.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

// clang-format off
OptionGroup TransportGuardOptions =
    OptionGroup("Options specific to algorithm \"transportguard\""
                " (in addition to those shared with \"transport\")")
  + Option("transportguard_overexplain",
           "set the weight \"mu\" and density ratio threshold \"rho\""
           " of the one-sided over-explanation guard"
           " (defaults: " + str(default_transportguard_mu) + " " + str(default_transportguard_rho) + ")")
    + Argument("mu").type_float(0.0f)
    + Argument("rho").type_float(1.0f);
// clang-format on

AngularKernel TransportGuard::kernel = AngularKernel::TAN2;
float TransportGuard::cap = 1.0f; // tan^2(45 degrees) = 1
float TransportGuard::g = default_transport_complexity;
float TransportGuard::m = default_transportguard_mu;
float TransportGuard::r = default_transportguard_rho;

float TransportGuard::calculate(const std::vector<Correspondence::Fixel> &s,
                                const std::vector<Correspondence::Fixel> &rs,
                                const std::vector<Correspondence::Fixel> &t,
                                const std::vector<std::vector<index_type>> &inv_mapping,
                                const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  float result = Transport::transport_core(s, rs, t, inv_mapping, origins_per_remapped_fixel, kernel, cap, g);

  // One-sided guard against non-physical mass accumulation on a template fixel
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
    const float excess = rs[t_index].density() - r * t[t_index].density();
    if (excess > 0.0f)
      result += m * Math::pow2(excess);
  }

  return result;
}

void TransportGuard::set_constants(const AngularKernel kernel_in,
                                   const float angle_degrees,
                                   const float complexity,
                                   const float mu,
                                   const float rho) {
  kernel = kernel_in;
  const float cos_theta_star = static_cast<float>(std::cos(angle_degrees * Math::pi / 180.0));
  cap = angular_cost(cos_theta_star, kernel_in);
  g = complexity;
  m = mu;
  r = rho;
}

} // namespace MR::Fixel::Correspondence::Algorithms
