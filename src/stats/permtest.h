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

#include <mutex>

#include "progressbar.h"
#include "thread.h"
#include "math/stats/permutation.h"
#include "thread_queue.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {

      typedef float value_type;



      class PermutationStack { MEMALIGN(PermutationStack)
        public:
          PermutationStack (size_t num_permutations, size_t num_samples, std::string msg, bool include_default = true) :
            num_permutations (num_permutations),
            current_permutation (0),
            progress (msg, num_permutations) {
              Math::Stats::generate_permutations (num_permutations, num_samples, permutations, include_default);
            }

          size_t next () {
            std::lock_guard<std::mutex> lock (permutation_mutex);
            size_t index = current_permutation++;
            if (index < permutations.size())
              ++progress;
            return index;
          }
          const std::vector<size_t>& permutation (size_t index) const {
            return permutations[index];
          }

          const size_t num_permutations;

        protected:
          size_t current_permutation;
          ProgressBar progress;
          std::vector <std::vector<size_t> > permutations;
          std::mutex permutation_mutex;
      };




      /*! A class to pre-compute the empirical TFCE or CFE statistic image for non-stationarity correction */
      template <class StatsType, class EnchancementType>
        class PreProcessor { MEMALIGN(PreProcessor<StatsType,EnchancementType>)
          public:
            PreProcessor (PermutationStack& permutation_stack, const StatsType& stats_calculator,
                          const EnchancementType& enhancer, std::vector<double>& global_enhanced_sum,
                          std::vector<size_t>& global_enhanced_count) :
                            perm_stack (permutation_stack), stats_calculator (stats_calculator),
                            enhancer (enhancer), global_enhanced_sum (global_enhanced_sum),
                            global_enhanced_count (global_enhanced_count), enhanced_sum (global_enhanced_sum.size(), 0.0),
                            enhanced_count (global_enhanced_sum.size(), 0.0), stats (global_enhanced_sum.size()),
                            enhanced_stats (global_enhanced_sum.size()), mutex (new std::mutex()) {}

            ~PreProcessor ()
            {
              std::lock_guard<std::mutex> lock (*mutex);
              for (size_t i = 0; i < global_enhanced_sum.size(); ++i) {
                global_enhanced_sum[i] += enhanced_sum[i];
                global_enhanced_count[i] += enhanced_count[i];
              }
            }

            void execute ()
            {
              size_t index;
              while (( index = perm_stack.next() ) < perm_stack.num_permutations)
                process_permutation (index);
            }

          protected:

            void process_permutation (size_t index)
            {
              value_type max_stat = 0.0, min_stat = 0.0;
              stats_calculator (perm_stack.permutation (index), stats, max_stat, min_stat);
              enhancer (max_stat, stats, enhanced_stats);
              for (size_t i = 0; i < enhanced_stats.size(); ++i) {
                if (enhanced_stats[i] > 0.0) {
                  enhanced_sum[i] += enhanced_stats[i];
                  enhanced_count[i]++;
                }
              }
            }

            PermutationStack& perm_stack;
            StatsType stats_calculator;
            EnchancementType enhancer;
            std::vector<double>& global_enhanced_sum;
            std::vector<size_t>& global_enhanced_count;
            std::vector<double> enhanced_sum;
            std::vector<size_t> enhanced_count;
            std::vector<value_type> stats;
            std::vector<value_type> enhanced_stats;
            std::shared_ptr<std::mutex> mutex;
        };




        /*! A class to perform the permutation testing */
        template <class StatsType, class EnhancementType>
          class Processor { MEMALIGN(Processor<StatsType,EnhancementType>)
            public:
              Processor (PermutationStack& permutation_stack, const StatsType& stats_calculator,
                         const EnhancementType& enhancer, const std::shared_ptr<std::vector<double> >& empirical_enhanced_statistics,
                         const std::vector<value_type>& default_enhanced_statistics, const std::shared_ptr<std::vector<value_type> >& default_enhanced_statistics_neg,
                         Eigen::Matrix<value_type, Eigen::Dynamic, 1>& perm_dist_pos, std::shared_ptr<Eigen::Matrix<value_type, Eigen::Dynamic, 1> >& perm_dist_neg,
                         std::vector<size_t>& global_uncorrected_pvalue_counter, std::shared_ptr<std::vector<size_t> >& global_uncorrected_pvalue_counter_neg) :
                           perm_stack (permutation_stack), stats_calculator (stats_calculator),
                           enhancer (enhancer), empirical_enhanced_statistics (empirical_enhanced_statistics),
                           default_enhanced_statistics (default_enhanced_statistics), default_enhanced_statistics_neg (default_enhanced_statistics_neg),
                           statistics (stats_calculator.num_elements()), enhanced_statistics (stats_calculator.num_elements()),
                           uncorrected_pvalue_counter (stats_calculator.num_elements(), 0),
                           perm_dist_pos (perm_dist_pos), perm_dist_neg (perm_dist_neg),
                           global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
                           global_uncorrected_pvalue_counter_neg (global_uncorrected_pvalue_counter_neg) {
                             if (global_uncorrected_pvalue_counter_neg)
                               uncorrected_pvalue_counter_neg.reset (new std::vector<size_t>(stats_calculator.num_elements(), 0));
              }


              ~Processor () {
                for (size_t i = 0; i < stats_calculator.num_elements(); ++i) {
                  global_uncorrected_pvalue_counter[i] += uncorrected_pvalue_counter[i];
                  if (global_uncorrected_pvalue_counter_neg)
                    (*global_uncorrected_pvalue_counter_neg)[i] = (*uncorrected_pvalue_counter_neg)[i];
                }
              }

              void execute ()
              {
                size_t index;
                while (( index = perm_stack.next() ) < perm_stack.num_permutations)
                    process_permutation (index);
              }


            protected:

              void process_permutation (size_t index)
              {
                value_type max_stat = 0.0, min_stat = 0.0;
                stats_calculator (perm_stack.permutation (index), statistics, max_stat, min_stat);
                perm_dist_pos(index) = enhancer (max_stat, statistics, enhanced_statistics);

                if (empirical_enhanced_statistics) {
                  perm_dist_pos(index) = 0.0;
                  for (size_t i = 0; i < enhanced_statistics.size(); ++i) {
                    enhanced_statistics[i] /= (*empirical_enhanced_statistics)[i];
                    if (enhanced_statistics[i] > perm_dist_pos(index))
                      perm_dist_pos(index) = enhanced_statistics[i];
                  }
                }

                for (size_t i = 0; i < enhanced_statistics.size(); ++i) {
                  if (default_enhanced_statistics[i] > enhanced_statistics[i])
                    uncorrected_pvalue_counter[i]++;
                }

                // Compute the opposite contrast
                if (perm_dist_neg) {
                  for (size_t i = 0; i < statistics.size(); ++i)
                    statistics[i] = -statistics[i];

                  (*perm_dist_neg)(index) = enhancer (-min_stat, statistics, enhanced_statistics);

                  if (empirical_enhanced_statistics) {
                    (*perm_dist_neg)(index) = 0.0;
                    for (size_t i = 0; i < enhanced_statistics.size(); ++i) {
                      enhanced_statistics[i] /= (*empirical_enhanced_statistics)[i];
                      if (enhanced_statistics[i] > (*perm_dist_neg)(index))
                        (*perm_dist_neg)(index) = enhanced_statistics[i];
                    }
                  }

                  for (size_t i = 0; i < enhanced_statistics.size(); ++i) {
                    if ((*default_enhanced_statistics_neg)[i] > enhanced_statistics[i])
                      (*uncorrected_pvalue_counter_neg)[i]++;
                  }
                }
              }


              PermutationStack& perm_stack;
              StatsType stats_calculator;
              EnhancementType enhancer;
              std::shared_ptr<std::vector<double> > empirical_enhanced_statistics;
              const std::vector<value_type>& default_enhanced_statistics;
              const std::shared_ptr<std::vector<value_type> > default_enhanced_statistics_neg;
              std::vector<value_type> statistics;
              std::vector<value_type> enhanced_statistics;
              std::vector<size_t> uncorrected_pvalue_counter;
              std::shared_ptr<std::vector<size_t> > uncorrected_pvalue_counter_neg;
              Eigen::Matrix<value_type, Eigen::Dynamic, 1>& perm_dist_pos;
              std::shared_ptr<Eigen::Matrix<value_type, Eigen::Dynamic, 1> > perm_dist_neg;
              std::vector<size_t>& global_uncorrected_pvalue_counter;
              std::shared_ptr<std::vector<size_t> > global_uncorrected_pvalue_counter_neg;
        };


        // Precompute the empircal test statistic for non-stationarity adjustment
        template <class StatsType, class EnhancementType>
          inline void precompute_empirical_stat (const StatsType& stats_calculator, const EnhancementType& enhancer,
                                                 size_t num_permutations, std::vector<double>& empirical_statistic)
          {
            std::vector<size_t> global_enhanced_count (empirical_statistic.size(), 0);
            PermutationStack preprocessor_permutations (num_permutations,
                                                        stats_calculator.num_subjects(),
                                                        "precomputing empirical statistic for non-stationarity adjustment...", false);
            {
              PreProcessor<StatsType, EnhancementType> preprocessor (preprocessor_permutations, stats_calculator, enhancer,
                                                                     empirical_statistic, global_enhanced_count);
              auto preprocessor_threads = Thread::run (Thread::multi (preprocessor), "preprocessor threads");
            }
            for (size_t i = 0; i < empirical_statistic.size(); ++i) {
              if (global_enhanced_count[i] > 0)
                empirical_statistic[i] /= static_cast<double> (global_enhanced_count[i]);
            }
          }



          // Precompute the default statistic image and enhanced statistic. We need to precompute this for calculating the uncorrected p-values.
          template <class StatsType, class EnhancementType>
            inline void precompute_default_permutation (const StatsType& stats_calculator, const EnhancementType& enhancer,
                                                        const std::shared_ptr<std::vector<double> >& empirical_enhanced_statistic,
                                                        std::vector<value_type>& default_enhanced_statistics, std::shared_ptr<std::vector<value_type> >& default_enhanced_statistics_neg,
                                                        std::vector<value_type>& default_statistics)
            {
              std::vector<size_t> default_labelling (stats_calculator.num_subjects());
              for (size_t i = 0; i < default_labelling.size(); ++i)
                default_labelling[i] = i;
              value_type max_stat = 0.0, min_stat = 0.0;
              stats_calculator (default_labelling, default_statistics, max_stat, min_stat);
              max_stat = enhancer (max_stat, default_statistics, default_enhanced_statistics);

              if (empirical_enhanced_statistic) {
                for (size_t i = 0; i < default_statistics.size(); ++i)
                  default_enhanced_statistics[i] /= (*empirical_enhanced_statistic)[i];
              }

              // Compute the opposite contrast
              if (default_enhanced_statistics_neg) {
                for (size_t i = 0; i < default_statistics.size(); ++i)
                  default_statistics[i] = -default_statistics[i];

                max_stat = enhancer (-min_stat, default_statistics, *default_enhanced_statistics_neg);

                if (empirical_enhanced_statistic) {
                  for (size_t i = 0; i < default_statistics.size(); ++i)
                    (*default_enhanced_statistics_neg)[i] /= (*empirical_enhanced_statistic)[i];
                }
                // revert default_statistics to positive contrast for output
                for (size_t i = 0; i < default_statistics.size(); ++i)
                  default_statistics[i] = -default_statistics[i];
              }
            }




        template <class StatsType, class EnhancementType>
          inline void run_permutations (const StatsType& stats_calculator, const EnhancementType& enhancer, size_t num_permutations,
                                        const std::shared_ptr<std::vector<double> >& empirical_enhanced_statistic,
                                        const std::vector<value_type>& default_enhanced_statistics, const std::shared_ptr<std::vector<value_type> >& default_enhanced_statistics_neg,
                                        Eigen::Matrix<value_type, Eigen::Dynamic, 1>& perm_dist_pos, std::shared_ptr<Eigen::Matrix<value_type, Eigen::Dynamic, 1> >& perm_dist_neg,
                                        std::vector<value_type>& uncorrected_pvalues, std::shared_ptr<std::vector<value_type> >& uncorrected_pvalues_neg)
          {

            std::vector<size_t> global_uncorrected_pvalue_count (stats_calculator.num_elements(), 0);
            std::shared_ptr<std::vector<size_t> > global_uncorrected_pvalue_count_neg;
            if (perm_dist_neg)
              global_uncorrected_pvalue_count_neg.reset (new std::vector<size_t>  (stats_calculator.num_elements(), 0));

            {
              PermutationStack permutations (num_permutations,
                                             stats_calculator.num_subjects(),
                                             "running " + str(num_permutations) + " permutations...");

              Processor<StatsType, EnhancementType> processor (permutations, stats_calculator, enhancer,
                                                               empirical_enhanced_statistic,
                                                               default_enhanced_statistics, default_enhanced_statistics_neg,
                                                               perm_dist_pos, perm_dist_neg,
                                                               global_uncorrected_pvalue_count, global_uncorrected_pvalue_count_neg);
              auto threads = Thread::run (Thread::multi (processor), "permutation threads");
            }

            for (size_t i = 0; i < stats_calculator.num_elements(); ++i) {
              uncorrected_pvalues[i] = static_cast<value_type> (global_uncorrected_pvalue_count[i]) / static_cast<value_type> (num_permutations);
              if (perm_dist_neg)
                (*uncorrected_pvalues_neg)[i] = static_cast<value_type> ((*global_uncorrected_pvalue_count_neg)[i]) / static_cast<value_type> (num_permutations);
            }

          }
          //! @}

    }
  }
}

#endif
