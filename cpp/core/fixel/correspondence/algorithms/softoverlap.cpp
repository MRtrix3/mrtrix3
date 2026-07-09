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

#include "fixel/correspondence/algorithms/softoverlap.h"

#include "exception.h"

namespace MR::Fixel::Correspondence::Algorithms {

// Direct, data-driven fractional attribution by dixel-mask overlap.
//
// For each source fixel i, its overlap with target fixel j is the number of sampling directions
//   shared between their dixel masks, o_ij = |Omega_s[i] & Omega_t[j]|. The entirety of source i's
//   fibre density is then distributed across the overlapping targets in proportion to those
//   overlaps, i.e. with weight w_ij = o_ij / sum_k o_ik. Because the weights are normalised by the
//   source fixel's total overlap, a source fixel overlapping a single target contributes its full
//   density to that target irrespective of the overlap magnitude, whereas a source fixel
//   overlapping several targets is split between them by relative overlap.
//
// Dixel weights are uniform: the sampling set is near-uniform and FastLookupSet exposes no
//   quadrature weights (consistent with algorithm "maskoverlap"). Under FMLS mutual exclusivity a
//   dixel belongs to at most one lobe within each dataset, so no per-dixel sharing arises.
std::vector<std::vector<Mapping::Entry>> SoftOverlap::operator()(const voxel_t &,
                                                                 const std::vector<Correspondence::Fixel> &s,
                                                                 const std::vector<Correspondence::Fixel> &t,
                                                                 const std::vector<dixel_mask_t> &s_masks,
                                                                 const std::vector<dixel_mask_t> &t_masks) const {
  assert(s_masks.size() == s.size());
  assert(t_masks.size() == t.size());

  // Forward mapping: one (initially empty) attribution list per target fixel
  std::vector<std::vector<Mapping::Entry>> result(t.size());

  // Number of directions shared between the current source lobe and each target lobe;
  //   reused across source fixels to avoid per-fixel reallocation
  std::vector<Eigen::Index> overlap(t.size(), 0);
  for (index_type is = 0; is != s.size(); ++is) {
    Eigen::Index total = 0;
    for (index_type it = 0; it != t.size(); ++it) {
      const Eigen::Index shared =
          (s_masks[is].size() == t_masks[it].size()) ? (s_masks[is] && t_masks[it]).count() : Eigen::Index(0);
      overlap[it] = shared;
      total += shared;
    }
    // A source fixel that geometrically overlaps no target lobe has no correspondence;
    //   its fibre density is left unattributed
    if (total == 0)
      continue;
    for (index_type it = 0; it != t.size(); ++it) {
      if (overlap[it] == 0)
        continue;
      const float weight = static_cast<float>(overlap[it]) / static_cast<float>(total);
      result[it].push_back(Mapping::Entry{is, weight});
    }
  }

  return result;
}

std::vector<std::vector<Mapping::Entry>> SoftOverlap::operator()(const voxel_t &,
                                                                 const std::vector<Correspondence::Fixel> &,
                                                                 const std::vector<Correspondence::Fixel> &) const {
  assert(false && "SoftOverlap must be invoked through its dixel-mask-carrying overload");
  throw Exception("Algorithm \"softoverlap\" requires per-fixel dixel masks;" //
                  " internal error (mask-free invocation)");
}

} // namespace MR::Fixel::Correspondence::Algorithms
