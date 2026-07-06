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

// #define FIXELCORRESPONDENCE_TEST_PERVOXEL

#include "header.h"
#include "image.h"
#include "types.h"

#include "fixel/correspondence/adjacency.h"
#include "fixel/correspondence/algorithms/base.h"
#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/dp2cost.h"
#include "math/binomial.h"

namespace MR::App {
class OptionGroup;
} // namespace MR::App

namespace MR::Fixel::Correspondence::Algorithms {

extern App::OptionGroup CombinatorialOptions;

// Forward declarations for explicit template instantiation
class ISMRM2018;
class POT;
class RS2023;
class Transport;
class TransportDisp;
class Agreement;
class TransportGuard;
class MegaCost;
class MaskOverlap;

// Base class to handle the combinatorial aspects of both
//   what was presented at ISMRM2018 and new proposed expression
template <class CostFunctor> class Combinatorial : public Base {

public:
  Combinatorial(const index_type max_origins_per_target,
                const index_type max_objectives_per_source,
                const Header &H_cost)
      : max_origins_per_target(max_origins_per_target),
        max_objectives_per_source(max_objectives_per_source)
#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
        ,
        mutex(new std::mutex)
#endif
  {
    cost_image = Image<float>::scratch(H_cost, "scratch image containing minimal cost function per voxel");
  }

  virtual ~Combinatorial() {}

  // Default compile-time trait: a cost functor consumes dixel masks only if it redefines this.
  // Named distinctly from the runtime query requires_masks() to avoid a same-name data/function clash.
  static constexpr bool consumes_masks = false;

  // Runtime query used by the Matcher to decide whether to open and load the dixel-mask images.
  bool requires_masks() const final { return CostFunctor::consumes_masks; }

  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t) const final;

  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t,
                                                      const std::vector<dixel_mask_t> &s_masks,
                                                      const std::vector<dixel_mask_t> &t_masks) const final;

protected:
  const index_type max_origins_per_target;
  const index_type max_objectives_per_source;

  static DP2Cost dp2cost;

  static std::atomic_flag fixel_count_warning_issued;
#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
  // Track across the entire image the situation that results in the
  //   largest number of combinations for which calculation of the
  //   cost function was required
  // (if fixel counts in any individual voxel are too high,
  //   computation time goes through the roof)
  std::shared_ptr<std::mutex> mutex;
  static uint64_t max_computed_combinations;
#endif
};

extern template class Combinatorial<ISMRM2018>;
extern template class Combinatorial<POT>;
extern template class Combinatorial<RS2023>;
extern template class Combinatorial<Transport>;
extern template class Combinatorial<TransportDisp>;
extern template class Combinatorial<Agreement>;
extern template class Combinatorial<TransportGuard>;
extern template class Combinatorial<MegaCost>;
extern template class Combinatorial<MaskOverlap>;

} // namespace MR::Fixel::Correspondence::Algorithms
