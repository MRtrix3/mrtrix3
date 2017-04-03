/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __stats_permtest_h__
#define __stats_permtest_h__

#include <memory>
#include <mutex>

#include "app.h"
#include "progressbar.h"
#include "thread.h"
#include "thread_queue.h"
#include "math/math.h"
#include "math/stats/glm.h"
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"

#include "stats/enhance.h"
#include "stats/permstack.h"


#define DEFAULT_NUMBER_PERMUTATIONS 5000
#define DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY 5000


namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      typedef Math::Stats::value_type value_type;
      typedef Math::Stats::vector_type vector_type;



      const App::OptionGroup Options (const bool include_nonstationarity);


      /*! A class to pre-compute the empirical enhanced statistic image for non-stationarity correction */
      class PreProcessor { MEMALIGN (PreProcessor)
        public:
          PreProcessor (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                        const std::shared_ptr<EnhancerBase> enhancer,
                        vector_type& global_enhanced_sum,
                        vector<size_t>& global_enhanced_count);

          ~PreProcessor();

          bool operator() (const Permutation&);

        protected:
          std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator;
          std::shared_ptr<EnhancerBase> enhancer;
          vector_type& global_enhanced_sum;
          vector<size_t>& global_enhanced_count;
          vector_type enhanced_sum;
          vector<size_t> enhanced_count;
          vector_type stats;
          vector_type enhanced_stats;
          std::shared_ptr<std::mutex> mutex;
      };




      /*! A class to perform the permutation testing */
      class Processor { MEMALIGN (Processor)
        public:
          Processor (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                     const std::shared_ptr<EnhancerBase> enhancer,
                     const vector_type& empirical_enhanced_statistics,
                     const vector_type& default_enhanced_statistics,
                     const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                     vector_type& perm_dist_pos,
                     std::shared_ptr<vector_type> perm_dist_neg,
                     vector<size_t>& global_uncorrected_pvalue_counter,
                     std::shared_ptr< vector<size_t> > global_uncorrected_pvalue_counter_neg);

          ~Processor();

          bool operator() (const Permutation&);

        protected:
          std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator;
          std::shared_ptr<EnhancerBase> enhancer;
          const vector_type& empirical_enhanced_statistics;
          const vector_type& default_enhanced_statistics;
          const std::shared_ptr<vector_type> default_enhanced_statistics_neg;
          vector_type statistics;
          vector_type enhanced_statistics;
          vector<size_t> uncorrected_pvalue_counter;
          std::shared_ptr<vector<size_t> > uncorrected_pvalue_counter_neg;
          vector_type& perm_dist_pos;
          std::shared_ptr<vector_type> perm_dist_neg;

          vector<size_t>& global_uncorrected_pvalue_counter;
          std::shared_ptr<vector<size_t> > global_uncorrected_pvalue_counter_neg;
          std::shared_ptr<std::mutex> mutex;
      };




      // Precompute the empircal test statistic for non-stationarity adjustment
      void precompute_empirical_stat (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      PermutationStack& perm_stack, vector_type& empirical_statistic);




      // Precompute the default statistic image and enhanced statistic. We need to precompute this for calculating the uncorrected p-values.
      void precompute_default_permutation (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                           const std::shared_ptr<EnhancerBase> enhancer,
                                           const vector_type& empirical_enhanced_statistic,
                                           vector_type& default_enhanced_statistics,
                                           std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                                           vector_type& default_statistics);



      // Functions for running a large number of permutations
      // Different interfaces depending on how the permutations themselves are constructed:
      // - A pre-existing permutation stack class
      // - Pre-defined permutations (likely provided via a command-line option)
      // - A requested number of permutations
      void run_permutations (PermutationStack& perm_stack,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const vector_type& empirical_enhanced_statistic,
                             const vector_type& default_enhanced_statistics,
                             const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                             vector_type& perm_dist_pos,
                             std::shared_ptr<vector_type> perm_dist_neg,
                             vector_type& uncorrected_pvalues,
                             std::shared_ptr<vector_type> uncorrected_pvalues_neg);


      void run_permutations (const vector<vector<size_t>>& permutations,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const vector_type& empirical_enhanced_statistic,
                             const vector_type& default_enhanced_statistics,
                             const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                             vector_type& perm_dist_pos,
                             std::shared_ptr<vector_type> perm_dist_neg,
                             vector_type& uncorrected_pvalues,
                             std::shared_ptr<vector_type> uncorrected_pvalues_neg);


      void run_permutations (const size_t num_permutations,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const vector_type& empirical_enhanced_statistic,
                             const vector_type& default_enhanced_statistics,
                             const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                             vector_type& perm_dist_pos,
                             std::shared_ptr<vector_type> perm_dist_neg,
                             vector_type& uncorrected_pvalues,
                             std::shared_ptr<vector_type> uncorrected_pvalues_neg);


      //! @}

    }
  }
}

#endif
