/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __stats_permtest_h__
#define __stats_permtest_h__

#include <memory>
#include <mutex>

#include "progressbar.h"
#include "thread.h"
#include "thread_queue.h"
#include "math/math.h"
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"

#include "stats/enhance.h"
#include "stats/permstack.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      typedef Math::Stats::value_type value_type;
      typedef Math::Stats::vector_type vector_type;



      /*! A class to pre-compute the empirical enhanced statistic image for non-stationarity correction */
      template <class StatsType>
        class PreProcessor {
          public:
            PreProcessor (const StatsType& stats_calculator,
                          const std::shared_ptr<EnhancerBase> enhancer,
                          vector_type& global_enhanced_sum,
                          std::vector<size_t>& global_enhanced_count) :
                            stats_calculator (stats_calculator),
                            enhancer (enhancer), global_enhanced_sum (global_enhanced_sum),
                            global_enhanced_count (global_enhanced_count), enhanced_sum (vector_type::Zero (global_enhanced_sum.size())),
                            enhanced_count (global_enhanced_sum.size(), 0.0), stats (global_enhanced_sum.size()),
                            enhanced_stats (global_enhanced_sum.size()), mutex (new std::mutex()) {}

            ~PreProcessor ()
            {
              std::lock_guard<std::mutex> lock (*mutex);
              for (ssize_t i = 0; i < global_enhanced_sum.size(); ++i) {
                global_enhanced_sum[i] += enhanced_sum[i];
                global_enhanced_count[i] += enhanced_count[i];
              }
            }

            bool operator() (const Permutation& permutation)
            {
              stats_calculator (permutation.data, stats);
              (*enhancer) (stats, enhanced_stats);
              for (ssize_t i = 0; i < enhanced_stats.size(); ++i) {
                if (enhanced_stats[i] > 0.0) {
                  enhanced_sum[i] += enhanced_stats[i];
                  enhanced_count[i]++;
                }
              }
              return true;
            }

          protected:
            StatsType stats_calculator;
            std::shared_ptr<EnhancerBase> enhancer;
            vector_type& global_enhanced_sum;
            std::vector<size_t>& global_enhanced_count;
            vector_type enhanced_sum;
            std::vector<size_t> enhanced_count;
            vector_type stats;
            vector_type enhanced_stats;
            std::shared_ptr<std::mutex> mutex;
        };




        /*! A class to perform the permutation testing */
        template <class StatsType>
          class Processor {
            public:
              Processor (const StatsType& stats_calculator,
                         const std::shared_ptr<EnhancerBase> enhancer,
                         const vector_type& empirical_enhanced_statistics,
                         const vector_type& default_enhanced_statistics,
                         const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                         vector_type& perm_dist_pos,
                         std::shared_ptr<vector_type> perm_dist_neg,
                         std::vector<size_t>& global_uncorrected_pvalue_counter,
                         std::shared_ptr< std::vector<size_t> > global_uncorrected_pvalue_counter_neg) :
                           stats_calculator (stats_calculator),
                           enhancer (enhancer), empirical_enhanced_statistics (empirical_enhanced_statistics),
                           default_enhanced_statistics (default_enhanced_statistics), default_enhanced_statistics_neg (default_enhanced_statistics_neg),
                           statistics (stats_calculator.num_elements()), enhanced_statistics (stats_calculator.num_elements()),
                           uncorrected_pvalue_counter (stats_calculator.num_elements(), 0),
                           perm_dist_pos (perm_dist_pos), perm_dist_neg (perm_dist_neg),
                           global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
                           global_uncorrected_pvalue_counter_neg (global_uncorrected_pvalue_counter_neg),
                           mutex (new std::mutex())
              {
                if (global_uncorrected_pvalue_counter_neg)
                  uncorrected_pvalue_counter_neg.reset (new std::vector<size_t>(stats_calculator.num_elements(), 0));
              }


              ~Processor () {
                std::lock_guard<std::mutex> lock (*mutex);
                for (size_t i = 0; i < stats_calculator.num_elements(); ++i) {
                  global_uncorrected_pvalue_counter[i] += uncorrected_pvalue_counter[i];
                  if (global_uncorrected_pvalue_counter_neg)
                    (*global_uncorrected_pvalue_counter_neg)[i] = (*uncorrected_pvalue_counter_neg)[i];
                }
              }


              bool operator() (const Permutation& permutation)
              {
                stats_calculator (permutation.data, statistics);
                if (enhancer) {
                  perm_dist_pos[permutation.index] = (*enhancer) (statistics, enhanced_statistics);
                } else {
                  enhanced_statistics = statistics;
                  perm_dist_pos[permutation.index] = enhanced_statistics.maxCoeff();
                }

                if (empirical_enhanced_statistics.size()) {
                  perm_dist_pos[permutation.index] = 0.0;
                  for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
                    enhanced_statistics[i] /= empirical_enhanced_statistics[i];
                    perm_dist_pos[permutation.index] = std::max(perm_dist_pos[permutation.index], enhanced_statistics[i]);
                  }
                }

                for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
                  if (default_enhanced_statistics[i] > enhanced_statistics[i])
                    uncorrected_pvalue_counter[i]++;
                }

                // Compute the opposite contrast
                if (perm_dist_neg) {
                  statistics = -statistics;

                  (*perm_dist_neg)[permutation.index] = (*enhancer) (statistics, enhanced_statistics);

                  if (empirical_enhanced_statistics.size()) {
                    (*perm_dist_neg)[permutation.index] = 0.0;
                    for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
                      enhanced_statistics[i] /= empirical_enhanced_statistics[i];
                      (*perm_dist_neg)[permutation.index] = std::max ((*perm_dist_neg)[permutation.index], enhanced_statistics[i]);
                    }
                  }

                  for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
                    if ((*default_enhanced_statistics_neg)[i] > enhanced_statistics[i])
                      (*uncorrected_pvalue_counter_neg)[i]++;
                  }
                }
                return true;
              }

            protected:
              StatsType stats_calculator;
              std::shared_ptr<EnhancerBase> enhancer;
              const vector_type& empirical_enhanced_statistics;
              const vector_type& default_enhanced_statistics;
              const std::shared_ptr<vector_type> default_enhanced_statistics_neg;
              vector_type statistics;
              vector_type enhanced_statistics;
              std::vector<size_t> uncorrected_pvalue_counter;
              std::shared_ptr<std::vector<size_t> > uncorrected_pvalue_counter_neg;
              vector_type& perm_dist_pos;
              std::shared_ptr<vector_type> perm_dist_neg;

              std::vector<size_t>& global_uncorrected_pvalue_counter;
              std::shared_ptr<std::vector<size_t> > global_uncorrected_pvalue_counter_neg;
              std::shared_ptr<std::mutex> mutex;
        };


        // Precompute the empircal test statistic for non-stationarity adjustment
        template <class StatsType>
          void precompute_empirical_stat (const StatsType& stats_calculator, const std::shared_ptr<EnhancerBase> enhancer,
                                          const size_t num_permutations, vector_type& empirical_statistic)
          {
            std::vector<size_t> global_enhanced_count (empirical_statistic.size(), 0);
            PermutationStack permutations (num_permutations,
                                           stats_calculator.num_subjects(),
                                           "precomputing empirical statistic for non-stationarity adjustment...", false);
            {
              PreProcessor<StatsType> preprocessor (stats_calculator, enhancer, empirical_statistic, global_enhanced_count);
              Thread::run_queue (permutations, Permutation(), Thread::multi (preprocessor));
            }
            for (ssize_t i = 0; i < empirical_statistic.size(); ++i) {
              if (global_enhanced_count[i] > 0)
                empirical_statistic[i] /= static_cast<default_type> (global_enhanced_count[i]);
            }
          }



          // Precompute the default statistic image and enhanced statistic. We need to precompute this for calculating the uncorrected p-values.
          template <class StatsType>
            void precompute_default_permutation (const StatsType& stats_calculator,
                                                 const std::shared_ptr<EnhancerBase> enhancer,
                                                 const vector_type& empirical_enhanced_statistic,
                                                 vector_type& default_enhanced_statistics,
                                                 std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                                                 vector_type& default_statistics)
            {
              std::vector<size_t> default_labelling (stats_calculator.num_subjects());
              for (size_t i = 0; i < default_labelling.size(); ++i)
                default_labelling[i] = i;
              stats_calculator (default_labelling, default_statistics);
              (*enhancer) (default_statistics, default_enhanced_statistics);

              if (empirical_enhanced_statistic.size())
                default_enhanced_statistics /= empirical_enhanced_statistic;

              // Compute the opposite contrast
              if (default_enhanced_statistics_neg) {
                default_statistics = -default_statistics;

                (*enhancer) (default_statistics, *default_enhanced_statistics_neg);

                if (empirical_enhanced_statistic.size())
                  (*default_enhanced_statistics_neg) /= empirical_enhanced_statistic;

                // revert default_statistics to positive contrast for output
                default_statistics = -default_statistics;
              }
            }




        template <class StatsType>
          inline void run_permutations (const StatsType& stats_calculator,
                                        const std::shared_ptr<EnhancerBase> enhancer,
                                        const size_t num_permutations,
                                        const vector_type& empirical_enhanced_statistic,
                                        const vector_type& default_enhanced_statistics,
                                        const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                                        vector_type& perm_dist_pos,
                                        std::shared_ptr<vector_type> perm_dist_neg,
                                        vector_type& uncorrected_pvalues,
                                        std::shared_ptr<vector_type> uncorrected_pvalues_neg)
          {
            std::vector<size_t> global_uncorrected_pvalue_count (stats_calculator.num_elements(), 0);
            std::shared_ptr< std::vector<size_t> > global_uncorrected_pvalue_count_neg;
            if (perm_dist_neg)
              global_uncorrected_pvalue_count_neg.reset (new std::vector<size_t> (stats_calculator.num_elements(), 0));

            {
              PermutationStack permutations (num_permutations,
                                             stats_calculator.num_subjects(),
                                             "running " + str(num_permutations) + " permutations");

              Processor<StatsType> processor (stats_calculator, enhancer,
                                              empirical_enhanced_statistic,
                                              default_enhanced_statistics, default_enhanced_statistics_neg,
                                              perm_dist_pos, perm_dist_neg,
                                              global_uncorrected_pvalue_count, global_uncorrected_pvalue_count_neg);
              Thread::run_queue (permutations, Permutation(), Thread::multi (processor));
            }

            for (size_t i = 0; i < stats_calculator.num_elements(); ++i) {
              uncorrected_pvalues[i] = global_uncorrected_pvalue_count[i] / default_type(num_permutations);
              if (perm_dist_neg)
                (*uncorrected_pvalues_neg)[i] = (*global_uncorrected_pvalue_count_neg)[i] / default_type(num_permutations);
            }

          }
          //! @}

    }
  }
}

#endif
