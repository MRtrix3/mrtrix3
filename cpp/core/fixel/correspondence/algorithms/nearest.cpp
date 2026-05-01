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

#include "fixel/correspondence/algorithms/nearest.h"

#include <optional>

#include "app.h"
#include "fixel/correspondence/correspondence.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

std::vector<std::vector<Mapping::Entry>> Nearest::operator()(const voxel_t &,
                                                             const std::vector<Correspondence::Fixel> &s,
                                                             const std::vector<Correspondence::Fixel> &t) const {
  // Pass 1: find nearest source fixel for each target fixel
  std::vector<std::optional<index_type>> nearest(t.size());
  for (index_type it = 0; it != t.size(); ++it) {
    float max_dp = 0.0f;
    for (index_type is = 0; is != s.size(); ++is) {
      const float dp = t[it].absdot(s[is]);
      if (dp > max_dp) {
        max_dp = dp;
        nearest[it] = is;
      }
    }
    if (max_dp <= dp_threshold)
      nearest[it].reset();
  }
  // Pass 2: count how many targets selected each source fixel
  std::vector<index_type> objectives(s.size(), 0);
  for (const auto &ns : nearest)
    if (ns)
      ++objectives[*ns];
  // Pass 3: build result with weights
  std::vector<std::vector<Mapping::Entry>> result;
  result.reserve(t.size());
  for (const auto &ns : nearest) {
    if (ns)
      result.push_back({Mapping::Entry{*ns, 1.0f / static_cast<float>(objectives[*ns])}});
    else
      result.emplace_back();
  }
  return result;
}

OptionGroup NearestOptions = OptionGroup("Options specific to algorithms \"legacy\" and \"nearest\"") +
                             Option("angle",
                                    "maximum angle within which a corresponding fixel may be selected, in degrees "
                                    "(default: " +
                                        str(default_nearest_maxangle) + ")") +
                             Argument("value").type_float(0.0f, 90.0f);

} // namespace MR::Fixel::Correspondence::Algorithms
