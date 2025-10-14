/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"
#include "math/SH.h"
#include "types.h"

constexpr float sqrt_3_over_2 = 0.866025403784439;

namespace MR::DWI::Tractography::Algorithms {

using namespace MR::DWI::Tractography::Tracking;

FORCE_INLINE std::vector<Eigen::Vector3f> direction_grid(float max_angle, float spacing) {
  const float maxR = Math::pow2(max_angle / spacing);
  std::vector<Eigen::Vector3f> list;
  ssize_t extent = std::ceil(max_angle / spacing);

  for (ssize_t i = -extent; i <= extent; ++i) {
    for (ssize_t j = -extent; j <= extent; ++j) {
      float x = i + 0.5 * j;
      float y = sqrt_3_over_2 * j;
      float n = Math::pow2(x) + Math::pow2(y);
      if (n > maxR)
        continue;
      n = spacing * std::sqrt(n);
      float z = std::cos(n);
      if (n)
        n = spacing * std::sin(n) / n;
      list.push_back({n * x, n * y, z});
    }
  }

  return (list);
}

namespace {
class Pair {
public:
  Pair(float inclination, float amplitude) : incl(inclination), amp(amplitude) {}
  float incl, amp;
};
} // namespace

template <class Method> void calibrate(Method &method) {
  typename Method::Calibrate calibrate_func(method);
  const float sqrt3 = std::sqrt(3.0);
  const float max_angle = std::isfinite(method.S.max_angle_ho) ? method.S.max_angle_ho : method.S.max_angle_1o;

  std::vector<Pair> amps;
  for (float incl = 0.0; incl < max_angle; incl += 0.001) {
    amps.push_back(Pair(incl, calibrate_func(incl)));
    if (!std::isfinite(amps.back().amp) || amps.back().amp <= 0.0)
      break;
  }
  float zero = amps.back().incl;

  float N_min = Inf, theta_min = NaN, ratio = NaN;
  for (size_t i = 1; i < amps.size(); ++i) {
    float N = Math::pow2(max_angle);
    float Ns = N * (1.0 + amps[0].amp / amps[i].amp) / (2.0 * Math::pow2(zero));
    auto dirs = direction_grid(max_angle + amps[i].incl, sqrt3 * amps[i].incl);
    N = Ns + dirs.size();
    // std::cout << amps[i].incl << " " << amps[i].amp << " " << Ns << " " << dirs.size() << " " << Ns+dirs.size() <<
    // "\n";
    if (N > 0.0 && N < N_min) {
      N_min = N;
      theta_min = amps[i].incl;
      ratio = amps[0].amp / amps[i].amp;
    }
  }

  method.calibrate_list = direction_grid(max_angle + theta_min, sqrt3 * theta_min);
  method.calibrate_ratio = ratio;

  INFO("rejection sampling will use " + str(method.calibrate_list.size()) + " directions" + //
       " with a ratio of " + str(method.calibrate_ratio) +                                  //
       " (predicted number of samples per step = " + str(N_min) + ")");                     //
}

} // namespace MR::DWI::Tractography::Algorithms
