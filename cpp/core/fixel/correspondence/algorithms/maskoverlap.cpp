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

#include <algorithm>

#include "app.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

OptionGroup MaskOverlapOptions =
    OptionGroup("Options specific to algorithm \"maskoverlap\"") +
    Option("maskoverlap_complexity",
           "weight \"gamma\" applied to the linear penalty for merging multiple subject fixels into one template fixel"
           " or splitting one subject fixel across multiple template fixels"
           " (default: " +
               str(default_maskoverlap_gamma) + ")") +
    Argument("value").type_float(0.0);

float MaskOverlap::g = default_maskoverlap_gamma;

// Geometric, amplitude-free correspondence cost.
//
// Per candidate remapping, each remapped-subject fixel rs[k] (dixel mask Omega_rs[k], density
//   d_rs) is paired with template fixel t[k] (dixel mask Omega_t[k], density d_t). The cost
//   mirrors POT's family (matched transport + surplus + parsimony), with POT's directional
//   misalignment (1 - |cos theta|^p) replaced by (1 - overlap_fraction_k): the fraction of the
//   remapped-subject lobe left unexplained by its paired template lobe.
//
// Per-dixel sharing: a source fixel that splits across targets contributes its mask to several
//   remapped-subject fixels, so a dixel may be claimed by more than one Omega_rs[k]. Each dixel's
//   contribution is weighted by 1 / (number of remapped fixels claiming it) on the subject side,
//   and 1 / (number of template fixels claiming it) on the template side, so a shared dixel is not
//   double-counted. (Under FMLS mutual exclusivity the template multiplicity is 1, but it is
//   computed defensively.) Dixel weights are otherwise uniform: the 1281-direction sampling set is
//   near-uniform and FastLookupSet exposes no quadrature weights.
//
// The intersection is weighted by the subject-side multiplicity 1 / n_rs[i] (consistent with the
//   |Omega_rs[k]| denominator, so overlap_fraction_k is a genuine fraction in [0, 1]).
float MaskOverlap::calculate(const std::vector<Correspondence::Fixel> &s,
                             const std::vector<Correspondence::Fixel> &rs,
                             const std::vector<Correspondence::Fixel> &t,
                             const std::vector<std::vector<index_type>> &inv_mapping,
                             const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel,
                             const std::vector<dixel_mask_t> &rs_masks,
                             const std::vector<dixel_mask_t> &t_masks) {
  assert(rs.size() == t.size());
  assert(rs_masks.size() == rs.size());
  assert(t_masks.size() == t.size());
  assert(s.size() == inv_mapping.size());

  // Number of sampling directions; taken from the first template mask (every template fixel has
  //   at least one dixel). If no geometry is available the mask term vanishes.
  Eigen::Index num_dirs = 0;
  for (const auto &m : t_masks) {
    if (m.size() > 0) {
      num_dirs = m.size();
      break;
    }
  }

  float result = 0.0f;

  if (num_dirs > 0) {
    // Per-dixel multiplicities: how many remapped / template fixels claim each dixel
    Eigen::ArrayXf n_rs = Eigen::ArrayXf::Zero(num_dirs);
    Eigen::ArrayXf n_t = Eigen::ArrayXf::Zero(num_dirs);
    for (const auto &m : rs_masks) {
      if (m.size() == num_dirs)
        n_rs += m.cast<float>();
    }
    for (const auto &m : t_masks) {
      if (m.size() == num_dirs)
        n_t += m.cast<float>();
    }
    // Reciprocal multiplicities; zero where a dixel is claimed by no fixel (that entry is never
    //   read, as it corresponds to a mask bit that is false everywhere)
    const Eigen::ArrayXf inv_n_rs = (n_rs > 0.0f).select(1.0f / n_rs, 0.0f);
    const Eigen::ArrayXf inv_n_t = (n_t > 0.0f).select(1.0f / n_t, 0.0f);

    for (index_type k = 0; k != rs.size(); ++k) {
      const float d_t = t[k].density();
      const float d_rs = rs[k].density();

      float overlap_fraction = 0.0f;
      if (rs_masks[k].size() == num_dirs && t_masks[k].size() == num_dirs) {
        const float omega_rs = (rs_masks[k].cast<float>() * inv_n_rs).sum();
        if (omega_rs > 0.0f) {
          const float intersection = ((rs_masks[k] && t_masks[k]).cast<float>() * inv_n_rs).sum();
          overlap_fraction = intersection / omega_rs;
        }
      }

      // Matched mass is "transported" at a cost equal to the unexplained lobe fraction
      const float matched_mass = std::min(d_t, d_rs);
      result += matched_mass * (1.0f - overlap_fraction);

      // Surplus mass on either side is created/destroyed at unit cost;
      //   this also covers unmatched template fixels (d_rs = 0 -> result += d_t)
      result += std::fabs(d_t - d_rs);

      // Linear penalty per "extra" subject fixel merged into this remapped fixel
      const int n_origins = static_cast<int>(origins_per_remapped_fixel[k]);
      if (n_origins > 1)
        result += g * d_t * static_cast<float>(n_origins - 1);
    }
  } else {
    // No mask geometry at all: fall back to the density-only surplus + parsimony terms
    for (index_type k = 0; k != rs.size(); ++k) {
      result += std::fabs(t[k].density() - rs[k].density());
      const int n_origins = static_cast<int>(origins_per_remapped_fixel[k]);
      if (n_origins > 1)
        result += g * t[k].density() * static_cast<float>(n_origins - 1);
    }
  }

  // Per-subject contributions: subject fixels with no objective contribute their full density,
  //   and subject fixels split across multiple template fixels incur a linear parsimony penalty
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    const float d_s = s[s_index].density();
    const std::size_t n_objectives = inv_mapping[s_index].size();
    if (n_objectives == 0)
      result += d_s;
    else if (n_objectives > 1)
      result += g * d_s * static_cast<float>(n_objectives - 1);
  }

  return result;
}

void MaskOverlap::set_gamma(const float gamma_in) { g = gamma_in; }

} // namespace MR::Fixel::Correspondence::Algorithms
