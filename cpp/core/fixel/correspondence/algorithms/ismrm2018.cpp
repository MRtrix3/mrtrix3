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

#include "fixel/correspondence/algorithms/ismrm2018.h"

namespace MR::Fixel::Correspondence::Algorithms {

float ISMRM2018::calculate(const std::vector<Correspondence::Fixel> &s,
                           const std::vector<Correspondence::Fixel> &rs,
                           const std::vector<Correspondence::Fixel> &t,
                           const std::vector<std::vector<index_type>> &inv_mapping,
                           const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());
  float result = 0.0f;
  for (index_type index = 0; index != rs.size(); ++index) {
    if (rs[index].density()) {
      // Differences in fixel orientation contribute in such a way that
      //   angles of greater than 45 degrees are penalised more severely
      //   than would be leaving those fixels unmatched
      result += Math::pow2(t[index].density() - rs[index].density()) * dp2cost(t[index].absdot(rs[index]));
    } else {
      result += Math::pow2(t[index].density());
    }
  }

  // Need to find source fixels that did not contribute to any remapped fixel
  for (index_type index = 0; index != s.size(); ++index) {
    if (inv_mapping[index].empty())
      result += Math::pow2(s[index].density());
  }

  return result;
}

} // namespace MR::Fixel::Correspondence::Algorithms
