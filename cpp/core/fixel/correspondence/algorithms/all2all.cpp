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
