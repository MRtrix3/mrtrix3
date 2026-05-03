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

#include "fixel/correspondence/algorithms/combinatorial.h"

#include "app.h"
#include "fixel/correspondence/algorithms/in2023.h"
#include "fixel/correspondence/algorithms/ismrm2018.h"
#include "fixel/correspondence/algorithms/pot.h"

namespace MR::Fixel::Correspondence::Algorithms {

using namespace App;

template <class CostFunctor>
std::vector<std::vector<Mapping::Entry>> Combinatorial<CostFunctor>::operator()(
    const voxel_t &v, const std::vector<Correspondence::Fixel> &s, const std::vector<Correspondence::Fixel> &t) const {
  if (std::max(s.size(), t.size()) > max_fixels_for_no_combinatorial_warning &&
      !fixel_count_warning_issued.test_and_set(std::memory_order_relaxed)) {
    WARN("Excessive fixel counts can currently lead to prohibitively long execution times; "
         "suggest limiting maximal fixel count per voxel to no greater than " +
         str(max_fixels_for_no_combinatorial_warning));
  }

  // For each remapped source fixel, these are the source fixels
  //   from which they could possibly be derived
  std::vector<std::vector<index_type>> remapping_origins;

  // May need to forbid certain mappings due to source fixels not forming a
  //   cohesive cluster (based on connectivity in the convex set)
  const Correspondence::Adjacency adjacency_s(s);

  // Worst-case scenario of the number of different combinations of source
  //   fixels that could be mapped to any one remapped fixel
  // 2 ^ (number_of_source_fixels)
  const uint32_t max_src_fixel_combinations = static_cast<uint32_t>(1) << s.size();

#if defined(FIXELCORRESPONDENCE_TEST_COMBINATORICS) || !defined(NDEBUG)
  // Only combinations where no target fixel draws from
  //   more than "max_origins_per_target" source fixels are considered;
  //   here we want to ensure that the generated number of
  //   combinations matches with theory
  uint32_t permissible_src_fixel_combinations = max_src_fixel_combinations;
  for (index_type r = s.size(); r > max_origins_per_target; --r)
    permissible_src_fixel_combinations -= Math::n_choose_k(static_cast<index_type>(s.size()), r);
#endif

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
  uint32_t adjacency_rejection_count = 0;
#endif

#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
  std::cerr << "\n\n\n\nBuilding " << permissible_src_fixel_combinations << " permissible of "
            << max_src_fixel_combinations << " maximum combinations for " << s.size() << " source fixels:\n";
#endif
  for (uint32_t code = 0; code != max_src_fixel_combinations; ++code) {
    std::vector<index_type> fixels;
#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
    std::cerr << "[ ";
#endif
    for (index_type f = 0; f != s.size(); ++f) {
      if ((static_cast<uint32_t>(1) << f) & code) {
        fixels.push_back(f);
#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
        std::cerr << f << " ";
#endif
      }
    }
#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
    std::cerr << "]";
#endif
    // Don't allow more than "max_origins_per_target" source fixels to contribute toward a single target fixel
    if (fixels.size() <= max_origins_per_target) {
      // Don't allow template fixels to select sets of source fixels that include disconnections
      if (adjacency_s(fixels)) {
        remapping_origins.push_back(std::move(fixels));
#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
        std::cerr << "\n";
#endif
#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
      } else {
#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
        std::cerr << " <- REJECTED (adjacency)\n";
#endif
        ++adjacency_rejection_count;
#endif
      }
#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
    } else {
      std::cerr << " <- REJECTED (origins per target)\n";
#endif
    }
  }

#ifndef NDEBUG
  // This equivalence only holds if sets of source fixels were not excluded due to
  //   the adjacency requirement
  if (s.size() < min_dirs_to_enforce_adjacency)
    assert(remapping_origins.size() == permissible_src_fixel_combinations);
#endif

#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
  if (s.size() >= min_dirs_to_enforce_adjacency)
    std::cerr << "Of a possible " << permissible_src_fixel_combinations << " source fixel combinations, "
              << adjacency_rejection_count << " were rejected due to adjacency requirements\n";
#endif

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
  const uint64_t worstcase_total_combinations = std::pow<uint64_t>(max_src_fixel_combinations, t.size());
#endif
#if defined(FIXELCORRESPONDENCE_TEST_COMBINATORICS) || !defined(NDEBUG)
  const uint64_t total_combinations_after_origin_limit = std::pow<uint64_t>(remapping_origins.size(), t.size());
#endif

  /////////////////////////////////////////////////////////////
  // Data that will be utilised for every unique combination //
  /////////////////////////////////////////////////////////////

  // Each template fixel indexes into "remapping_origins"
  std::vector<uint32_t> mapping(t.size(), 0);

  Eigen::Array<float, Eigen::Dynamic, 1> source_fixel_multipliers(s.size());
  // Note: Signed type used to permit 8-bit Math::pow2() following subtraction of 1 from 0
  //   within some CostFunctors
  Eigen::Array<int8_t, Eigen::Dynamic, 1> origins_per_remapped_fixel(t.size());
  bool skip;
  index_type increment_index_for_skip;
  float cost;
  dir_t mean_direction;
  float sum_densities;

  // Required for enforcing the criterion where if a single source fixel maps to
  //   multiple template fixels, those template fixels must not be disconnected
  //   from one another in the space of convex hull adjacency
  std::vector<std::vector<index_type>> inv_mapping(s.size());
  const Correspondence::Adjacency adjacency_t(t);

#if defined(FIXELCORRESPONDENCE_TEST_COMBINATORICS) || !defined(NDEBUG)
  uint64_t cost_counter = 0;
  uint64_t skip_trigger_counter = 0;
  uint64_t skipped_entirely_counter = 0;
#endif

  std::vector<std::vector<Mapping::Entry>> result;
  float min_cost = std::numeric_limits<float>::infinity();

  do {

    origins_per_remapped_fixel.setZero();

#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
    std::cerr << "\nMapping being tested: ";
    for (const auto &i : mapping) {
      std::cerr << " [ ";
      if (remapping_origins[i].size()) {
        for (auto j : remapping_origins[i])
          std::cerr << j << " ";
      } else {
        std::cerr << "{} ";
      }
      std::cerr << "] ";
    }
    std::cerr << "\n";
#endif

    for (auto &i : inv_mapping)
      i.clear();
    for (index_type it = 0; it != t.size(); ++it) {
      for (const auto &is : remapping_origins[mapping[it]])
        inv_mapping[is].push_back(it);
    }

#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
    std::cerr << "Corresponding inverse mapping: ";
    for (const auto &i : inv_mapping) {
      std::cerr << " [ ";
      if (i.size()) {
        for (auto j : i)
          std::cerr << j << " ";
      } else {
        std::cerr << "{} ";
      }
      std::cerr << "] ";
    }
    std::cerr << "\n";
#endif

    increment_index_for_skip = 0;
    skip = false;
    for (index_type is = 0; is != s.size(); ++is) {
      if (inv_mapping[is].size() > max_objectives_per_source) {
        skip = true;
        // Smallest template fixel index is guaranteed to be at the end of the list
        //   (we can only do the most conservative increment given the conflict)
        increment_index_for_skip = std::max(increment_index_for_skip, inv_mapping[is].front());
      }
      if (!adjacency_t(inv_mapping[is])) {
        skip = true;
        // Can only use the illegitimacy of this mapping w.r.t. adjacency of template fixels
        //   if the number of template fixels to which this subject fixel maps is equal to
        //   the maximum permissible number; otherwise, it'd be possible for some other mapping
        //   of the subject fixel to incorporate another template fixel that bridges the disconnection
        if (inv_mapping[is].size() == max_objectives_per_source)
          increment_index_for_skip = std::max(increment_index_for_skip, inv_mapping[is].front());
        break;
      }
    }

#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
    std::cerr << "Remapping objectives per source fixel: [";
    for (const auto &i : inv_mapping)
      std::cerr << " " << i.size();
    std::cerr << " ]\n";
#endif

    if (skip) {

#if defined(FIXELCORRESPONDENCE_TEST_COMBINATORICS) || !defined(NDEBUG)
      ++skip_trigger_counter;
      uint64_t block_position = 0;
      uint64_t block_size = 1;
      for (index_type i = 0; i != increment_index_for_skip; ++i) {
        block_position += static_cast<uint64_t>(mapping[i]) * block_size;
        block_size *= remapping_origins.size();
      }
      skipped_entirely_counter += block_size - 1 - block_position;
      // skipped_entirely_counter += std::pow<uint64_t>(remapping_origins.size(), increment_index_for_skip) - 1;
#endif

      // At least one source fixel is contributing to more than "max_objectives_per_source"
      //   remapped fixels. When this occurs, we need to increment the
      //   mapping for this source fixel, and also reset mapping for all lower
      //   fixels (since we don't need to test all possible combinations;
      //   we know from the higher fixels that the mapping is invalid)
      for (index_type it = 0; it != increment_index_for_skip; ++it)
        mapping[it] = 0;

    } else {

#if defined(FIXELCORRESPONDENCE_TEST_COMBINATORICS) || !defined(NDEBUG)
      ++cost_counter;
#endif

      // Fibre density of each source fixel is distributed equally
      //   among the remapped fixels to which it contributes;
      //   its contribution toward remapped fixel orientations is
      //   similarly decreased
      for (index_type is = 0; is != s.size(); ++is)
        source_fixel_multipliers[is] = 1.0f / static_cast<float>(inv_mapping[is].size());

#ifdef FIXELCORRESPONDENCE_TEST_PERVOXEL
      std::cerr << "Source fixel multipliers: [ " << source_fixel_multipliers.transpose() << " ]\n";
#endif

      // Loops over remapped source fixels
      std::vector<Correspondence::Fixel> rs;
      for (index_type rs_index = 0; rs_index != t.size(); ++rs_index) {

        // This is the list of source fixels from which data shall be drawn
        //   in the construction of this remapped source fixel
        const std::vector<index_type> &origin_fixels(remapping_origins[mapping[rs_index]]);

        mean_direction.setZero();
        sum_densities = 0.0f;
        for (const auto &s_index : origin_fixels) {
          mean_direction += s[s_index].dir() * source_fixel_multipliers[s_index] * s[s_index].density() *
                            (t[rs_index].dot(s[s_index]) < 0.0f ? -1.0f : 1.0f);
          sum_densities += source_fixel_multipliers[s_index] * s[s_index].density();
        }
        rs.emplace_back(Correspondence::Fixel(mean_direction.normalized(), sum_densities));

        origins_per_remapped_fixel[rs_index] = origin_fixels.size();
      }

      cost = static_cast<const CostFunctor *const>(this)->calculate(s, rs, t, inv_mapping, origins_per_remapped_fixel);

      if (cost < min_cost) {
        min_cost = cost;
        result.clear();
        for (index_type t_index = 0; t_index != t.size(); ++t_index) {
          std::vector<Mapping::Entry> entries;
          for (const index_type is : remapping_origins[mapping[t_index]])
            entries.push_back({is, source_fixel_multipliers[is]});
          result.push_back(std::move(entries));
        }
      }

      increment_index_for_skip = 0;
    }

    // Increments the mapping index; unless this was the last source for this target fixel,
    //   in which case set it back to 0 and increment the mapping for the next fixel
    while (++mapping[increment_index_for_skip] == remapping_origins.size() && ++increment_index_for_skip < t.size())
      mapping[increment_index_for_skip - 1] = 0;
  } while (mapping.back() != remapping_origins.size());

#ifndef NDEBUG
  assert(cost_counter + skip_trigger_counter + skipped_entirely_counter == total_combinations_after_origin_limit);
#endif

  Image<float> local_cost_image(cost_image);
  assign_pos_of(v).to(local_cost_image);
  local_cost_image.value() = min_cost;

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
  {
    std::lock_guard<std::mutex> lock(*mutex);
    if (cost_counter > max_computed_combinations) {
      max_computed_combinations = cost_counter;
      std::cerr << "\nMaximum computed combinations incremented to " << max_computed_combinations << " (" << s.size()
                << " x " << t.size() << "): "
                << "worst case was " << worstcase_total_combinations << "; " << total_combinations_after_origin_limit
                << " after restricting origins per target fixel"
#ifdef FIXELCORRESPONDENCE_ENFORCE_ADJACENCY
                << " & source fixel adjacency"
#endif
                << "; " << skip_trigger_counter << " skip triggers due to objectives per source fixel limit"
#ifdef FIXELCORRESPONDENCE_ENFORCE_ADJACENCY
                << " & target fixel adjacency"
#endif
                << ", with by-product of " << skipped_entirely_counter << " combinations never assessed\n";
    }
  }
#endif

  return result;
}

template <class CostFunctor> DP2Cost Combinatorial<CostFunctor>::dp2cost;
template <class CostFunctor> std::atomic_flag Combinatorial<CostFunctor>::fixel_count_warning_issued = ATOMIC_FLAG_INIT;
#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
template <class CostFunctor> uint64_t Combinatorial<CostFunctor>::max_computed_combinations = 0;
#endif

template class Combinatorial<ISMRM2018>;
template class Combinatorial<IN2023>;
template class Combinatorial<POT>;

// clang-format off
OptionGroup CombinatorialOptions = OptionGroup("Options applicable to all combinatorial-based algorithms")
  + Option("max_origins",
           "maximal number of origin source fixels for an individual target fixel"
           " (default: " + str(default_max_origins_per_target) + ")")
    + Argument("value").type_integer(1)

  + Option("max_objectives",
           "maximal number of objective target fixels for an individual source fixel"
           " (default: " + str(default_max_objectives_per_source) + ")")
    + Argument("value").type_integer(1)

  + Option("cost",
           "export a 3D image containing the optimal value of the relevant cost function in each voxel")
    + Argument("path").type_image_out();
// clang-format on

} // namespace MR::Fixel::Correspondence::Algorithms
