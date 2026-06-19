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

// -----------------------------------------------------------------------------
// TEMPORARY cost-function evaluation harness.
//
// For the combinatorial fixel-correspondence algorithms, the dominant cost is
//   generation of the complete set of candidate fixel re-mappings within each
//   voxel; the cost functions themselves are comparatively trivial. Re-running
//   the full enumeration once per cost function (and once per internal parameter
//   set of such) is therefore wasteful when the intent is to evaluate and tune
//   many cost functions against a fixed dataset.
//
// MegaCost is a single "cost function" (in the sense of Combinatorial<>) that
//   internally holds a list of every cost function and parameter set under
//   evaluation. The existing combinatorial enumeration is reused verbatim:
//   Combinatorial<MegaCost>::operator() generates the candidate mappings once
//   per voxel and invokes MegaCost::calculate() once per candidate; that single
//   call loops over all configurations, tracking the minimal-cost candidate for
//   each independently. After the threaded loop, one mapping per configuration
//   is exported.
//
// This file and the corresponding command are intended to be removed once the
//   cost-function design has been settled.
// -----------------------------------------------------------------------------

#include <string>
#include <vector>

#include "header.h"
#include "types.h"

#include "fixel/correspondence/algorithms/combinatorial.h"
#include "fixel/correspondence/algorithms/kernel.h"
#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/fixel.h"

namespace MR::Fixel::Correspondence::Algorithms {

/// @brief A single cost function together with a concrete set of internal parameter values.
///
/// All parameters across all cost-function families are stored as members (not statics),
///   so that many configurations can be evaluated in parallel within a single pass.
///   Only the members relevant to a configuration's family are consulted by evaluate().
struct CostConfig {
  enum class Family { ISMRM2018, POT, RS2023, TRANSPORT, TRANSPORTDISP, AGREEMENT, TRANSPORTGUARD };

  Family family;
  std::string name; // used as the output .npz filename (without extension)

  AngularKernel kernel = AngularKernel::TAN2;
  float gamma = 0.0f;  // POT / transport-family complexity (linear parsimony)
  float alpha = 0.0f;  // RS2023 density-difference weight
  float beta = 0.0f;   // RS2023 / agreement squared-parsimony weight
  float sigma = 1.0f;  // agreement contrast-protection scale
  float lambda = 0.0f; // transportdisp within-fixel dispersion weight
  float mu = 0.0f;     // transportguard over-explanation weight
  float rho = 2.0f;    // transportguard over-explanation density ratio
  float angle = 45.0f; // transport-family threshold angle theta* (degrees); recorded for the manifest
  float cap = 1.0f;    // angular cost a(theta*); precomputed from angle + kernel at construction

  /// @brief Evaluate this configuration's cost for one candidate re-mapping.
  float evaluate(const std::vector<Correspondence::Fixel> &s,
                 const std::vector<Correspondence::Fixel> &rs,
                 const std::vector<Correspondence::Fixel> &t,
                 const std::vector<std::vector<index_type>> &inv_mapping,
                 const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) const;
};

class MegaCost : public Combinatorial<MegaCost> {
public:
  MegaCost(const index_type max_origins_per_target, const index_type max_objectives_per_source, const Header &H_cost)
      : Combinatorial(max_origins_per_target, max_objectives_per_source, H_cost) {}

  // CRTP cost-function entry point invoked by Combinatorial<MegaCost>::operator() once per
  //   candidate re-mapping. Evaluates every configuration and updates the per-configuration
  //   best (minimal-cost) candidate held in the thread-local scratch.
  // Static (as required by the Combinatorial CRTP) and therefore reads only static state;
  //   per-thread mutable state lives in the thread_local scratch, and the immutable list of
  //   configurations is shared read-only.
  static float calculate(const std::vector<Correspondence::Fixel> &s,
                         const std::vector<Correspondence::Fixel> &rs,
                         const std::vector<Correspondence::Fixel> &t,
                         const std::vector<std::vector<index_type>> &inv_mapping,
                         const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel);

  /// @brief Enumerate all candidate re-mappings for one voxel exactly once, evaluating every
  ///   configuration, and return per configuration the inverse mapping (voxel-local target
  ///   fixel indices per source fixel) of that configuration's minimal-cost candidate.
  ///
  /// The returned reference is into thread-local scratch and is valid until the next call on
  ///   the same thread; the caller must consume it before invoking evaluate_all() again.
  /// The target fixel vector must be non-empty (guaranteed by the caller).
  const std::vector<std::vector<std::vector<index_type>>> &evaluate_all(
      const voxel_t &v, const std::vector<Correspondence::Fixel> &s, const std::vector<Correspondence::Fixel> &t) const;

  /// @brief The hard-coded list of cost functions and parameter sets to be evaluated.
  static const std::vector<CostConfig> &configs();

private:
  // Per-thread scratch holding, for the voxel currently being processed, the best (minimal)
  //   cost seen so far per configuration and the corresponding inverse mapping.
  struct Scratch {
    std::vector<float> best_cost;                               // [config]
    std::vector<std::vector<std::vector<index_type>>> best_inv; // [config][source][objective target indices]
    void reset(const size_t nconfig, const size_t nsource);
  };
  static thread_local Scratch scratch;
};

} // namespace MR::Fixel::Correspondence::Algorithms
