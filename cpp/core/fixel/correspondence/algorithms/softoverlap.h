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

#pragma once

#include "types.h"

#include "fixel/correspondence/algorithms/base.h"

namespace MR::Fixel::Correspondence::Algorithms {

/// @brief Non-combinatorial fractional dixel-mask-overlap correspondence.
///
/// Unlike the combinatorial algorithms, which select a single discrete mapping and divide each
///   source fixel's density equally among the target fixels it is assigned to, this algorithm
///   performs a direct, data-driven fractional attribution using the per-fixel dixel masks alone
///   (no FOD amplitude information).
///
/// Each source fixel distributes the entirety of its fibre density across the target fixels with
///   which its dixel mask overlaps, in proportion to the magnitude of each overlap. Writing the
///   overlap of source lobe i with target lobe j as the number of sampling directions shared
///   between their masks, o_ij = |Omega_s[i] & Omega_t[j]|, the fraction of source i's density
///   attributed to target j is w_ij = o_ij / sum_k o_ik. Consequently:
///   - a source fixel overlapping exactly one target contributes all of its density to that
///       target, however small the overlap;
///   - a source fixel straddling several targets is split between them by relative overlap;
///   - a source fixel overlapping no target has no correspondence and is left unattributed.
///
/// The normalisation is by the source fixel's total overlap across candidate targets, so the
///   attribution weights are not a Dice-style similarity: they depend only on the relative
///   overlaps, not on the absolute sizes of the lobes.
class SoftOverlap : public Base {
public:
  SoftOverlap() = default;
  ~SoftOverlap() override = default;

  bool requires_masks() const final { return true; }

  /// @brief Mask-carrying entry point; performs the fractional attribution.
  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t,
                                                      const std::vector<dixel_mask_t> &s_masks,
                                                      const std::vector<dixel_mask_t> &t_masks) const final;

protected:
  /// @brief Mask-free entry point; never invoked (requires_masks() == true) and present only to
  ///   satisfy the Base interface. Attribution is undefined without dixel masks, so it throws.
  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t) const final;
};

} // namespace MR::Fixel::Correspondence::Algorithms
