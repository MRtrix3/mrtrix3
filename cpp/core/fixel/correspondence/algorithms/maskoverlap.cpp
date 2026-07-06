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

#include "fixel/correspondence/algorithms/maskoverlap.h"

#include <limits>

namespace MR::Fixel::Correspondence::Algorithms {

// Stage-3 stub: the mask-carrying plumbing is exercised end-to-end, but no geometric cost is
//   computed yet. Returning NaN keeps the combinatorial enumeration crash-free (cost < min_cost
//   is always false), so every target fixel receives an empty mapping. The real cost follows.
float MaskOverlap::calculate(const std::vector<Correspondence::Fixel> &,
                             const std::vector<Correspondence::Fixel> &,
                             const std::vector<Correspondence::Fixel> &,
                             const std::vector<std::vector<index_type>> &,
                             const Eigen::Array<int8_t, Eigen::Dynamic, 1> &,
                             const std::vector<dixel_mask_t> &,
                             const std::vector<dixel_mask_t> &) {
  return std::numeric_limits<float>::quiet_NaN();
}

} // namespace MR::Fixel::Correspondence::Algorithms
