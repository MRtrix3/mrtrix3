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

#include "header.h"

#include "fixel/correspondence/algorithms/combinatorial.h"

namespace MR::App {
class OptionGroup;
} // namespace MR::App

namespace MR::Fixel::Correspondence::Algorithms {

extern App::OptionGroup MaskOverlapOptions;

/// @brief Geometric dixel-mask-overlap correspondence cost function.
///
/// Scores correspondence by the overlap of the segmented lobes on the sphere (dixel masks)
///   rather than by an angle between mean directions, using no FOD amplitude information.
/// Distinguished at compile time by consumes_masks == true, which drives the combinatorial
///   framework to construct remapped-subject dixel masks per candidate mapping and pass both
///   those and the template dixel masks to calculate().
///
/// The cost reuses the partial-optimal-transport family skeleton of POT (matched transport +
///   surplus + parsimony), but replaces POT's directional (1 - |cos theta|^p) misalignment term
///   with a mask-based (1 - overlap_fraction): the fraction of each remapped-subject lobe left
///   unexplained by its paired template lobe. Per-dixel contributions are normalized by the
///   number of fixels claiming that dixel, so a dixel shared across fixels is not double-counted.
class MaskOverlap : public Combinatorial<MaskOverlap> {
public:
  // Compile-time trait that gates dixel-mask construction in Combinatorial<MaskOverlap>.
  static constexpr bool consumes_masks = true;

  MaskOverlap(const index_type max_origins_per_target, const index_type max_objectives_per_source, const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel,
                         const std::vector<dixel_mask_t> &rs_masks,
                         const std::vector<dixel_mask_t> &t_masks);

  static void set_gamma(const float gamma);

protected:
  static float g;
};

} // namespace MR::Fixel::Correspondence::Algorithms
