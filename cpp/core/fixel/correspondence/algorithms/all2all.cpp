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

#include "fixel/correspondence/algorithms/all2all.h"

#ifdef FIXELCORRESPONDENCE_INCLUDE_ALL2ALL

namespace MR::Fixel::Correspondence::Algorithms {

std::vector<std::vector<Mapping::Entry>> All2All::operator()(const voxel_t &,
                                                             const std::vector<Correspondence::Fixel> &s,
                                                             const std::vector<Correspondence::Fixel> &t) const {
  std::vector<std::vector<Mapping::Entry>> result;
  std::vector<Mapping::Entry> all_s;
  const float weight = t.empty() ? 0.0f : 1.0f / static_cast<float>(t.size());
  for (index_type i = 0; i != s.size(); ++i)
    all_s.push_back({i, weight});
  result.assign(t.size(), all_s);
  return result;
}

} // namespace MR::Fixel::Correspondence::Algorithms

#endif
