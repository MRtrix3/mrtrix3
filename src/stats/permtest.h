/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __stats_permtest_h__
#define __stats_permtest_h__

#include <memory>
#include <mutex>

#include "app.h"
#include "progressbar.h"
#include "thread.h"
#include "thread_queue.h"
#include "types.h"
#include "math/math.h"
#include "math/stats/glm.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"

#include "stats/enhance.h"


#define DEFAULT_NUMBER_PERMUTATIONS 5000
#define DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY 5000


namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      using value_type = Math::Stats::value_type;
      using vector_type = Math::Stats::vector_type;
      using matrix_type = Math::Stats::matrix_type;
      using count_matrix_type = Eigen::Array<uint32_t, Eigen::Dynamic, Eigen::Dynamic>;



      /*! A class to pre-compute the empirical enhanced statistic image for non-stationarity correction */
      class PreProcessor { MEMALIGN (PreProcessor)
        public:
          PreProcessor (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                        const std::shared_ptr<EnhancerBase> enhancer,
                        const default_type skew,
                        matrix_type& global_enhanced_sum,
                        count_matrix_type& global_enhanced_count);

          ~PreProcessor();

          bool operator() (const Math::Stats::Shuffle&);

        protected:
          std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator;
          std::shared_ptr<EnhancerBase> enhancer;
          const default_type skew;
          matrix_type& global_enhanced_sum;
          count_matrix_type& global_enhanced_count;
          matrix_type enhanced_sum;
          count_matrix_type enhanced_count;
          matrix_type stats;
          matrix_type enhanced_stats;
          std::shared_ptr<std::mutex> mutex;
      };




      /*! A class to perform the permutation testing */
      class Processor { MEMALIGN (Processor)
        public:
          Processor (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                     const std::shared_ptr<EnhancerBase> enhancer,
                     const matrix_type& empirical_enhanced_statistics,
                     const matrix_type& default_enhanced_statistics,
                     matrix_type& null_dist,
                     count_matrix_type& global_null_dist_contributions,
                     count_matrix_type& global_uncorrected_pvalue_counter);

          ~Processor();

          bool operator() (const Math::Stats::Shuffle&);

        protected:
          std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator;
          std::shared_ptr<EnhancerBase> enhancer;
          const matrix_type& empirical_enhanced_statistics;
          const matrix_type& default_enhanced_statistics;
          matrix_type statistics;
          matrix_type enhanced_statistics;
          matrix_type& null_dist;
          count_matrix_type& global_null_dist_contributions;
          count_matrix_type null_dist_contribution_counter;
          count_matrix_type& global_uncorrected_pvalue_counter;
          count_matrix_type uncorrected_pvalue_counter;
          std::shared_ptr<std::mutex> mutex;
      };




      // Precompute the empircal test statistic for non-stationarity adjustment
      void precompute_empirical_stat (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      const default_type skew,
                                      matrix_type& empirical_statistic);




      // Precompute the default statistic image and enhanced statistic. We need to precompute this for calculating the uncorrected p-values.
      void precompute_default_permutation (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                                           const std::shared_ptr<EnhancerBase> enhancer,
                                           const matrix_type& empirical_enhanced_statistic,
                                           matrix_type& output_statistics,
                                           matrix_type& output_zstats,
                                           matrix_type& output_enhanced);



      // Functions for running a large number of permutations
      void run_permutations (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const matrix_type& empirical_enhanced_statistic,
                             const matrix_type& default_enhanced_statistics,
                             const bool fwe_strong,
                             matrix_type& perm_dist,
                             count_matrix_type& perm_dist_contributions,
                             matrix_type& uncorrected_pvalues);

      //! @}

    }
  }
}

#endif
