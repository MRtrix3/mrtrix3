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
class IN2023;

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

  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t) const final;

protected:
  const index_type max_origins_per_target;
  const index_type max_objectives_per_source;

  static DP2Cost dp2cost;

  // Derived class function to calculate cost function
  // CRTP to template out: Can't be calling a virtual function
  //   this regularly without severe slowdown...
  FORCE_INLINE static float calculate(const std::vector<Correspondence::Fixel> &s,
                                      const std::vector<Correspondence::Fixel> &rs,
                                      const std::vector<Correspondence::Fixel> &t,
                                      const std::vector<std::vector<index_type>> &inv_mapping,
                                      const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
    return CostFunctor::calculate(s, rs, t, inv_mapping, origins_per_remapped_fixel);
  }

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
extern template class Combinatorial<IN2023>;

} // namespace MR::Fixel::Correspondence::Algorithms
