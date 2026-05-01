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

#include "fixel/correspondence/algorithms/legacy.h"

namespace MR::Fixel::Correspondence::Algorithms {

std::vector<std::vector<Mapping::Entry>> Legacy::operator()(const voxel_t &,
                                                            const std::vector<Correspondence::Fixel> &s,
                                                            const std::vector<Correspondence::Fixel> &t) const {
  std::vector<std::vector<Mapping::Entry>> result;
  result.reserve(t.size());
  for (index_type it = 0; it != t.size(); ++it) {
    index_type closest_index = 0;
    float max_dp = 0.0f;
    for (index_type is = 0; is != s.size(); ++is) {
      const float dp = t[it].absdot(s[is]);
      if (dp > max_dp) {
        max_dp = dp;
        closest_index = is;
      }
    }
    if (max_dp > dp_threshold)
      result.push_back({Mapping::Entry{closest_index, 1.0f}});
    else
      result.emplace_back();
  }
  return result;
}

} // namespace MR::Fixel::Correspondence::Algorithms
