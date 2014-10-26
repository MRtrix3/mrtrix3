/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt and Donald Tournier 23/07/11.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __stats_tfce_h__
#define __stats_tfce_h__

#include <gsl/gsl_linalg.h>

#include "math/vector.h"
#include "math/matrix.h"
#include "math/stats/permutation.h"
#include "image/filter/connected_components.h"
#include "thread/queue.h"

namespace MR
{
  namespace Stats
  {
    namespace TFCE
    {

      typedef float value_type;



      /** \addtogroup Statistics
      @{ */
      class ClusterSize {
        public:
          ClusterSize (const Image::Filter::Connector& connector, value_type cluster_forming_threshold) :
                      connector (connector), cluster_forming_threshold (cluster_forming_threshold) { }

          value_type operator() (const value_type unused, const std::vector<value_type>& stats,
                                 std::vector<value_type>* get_cluster_sizes)
          {
            std::vector<Image::Filter::cluster> clusters;
            std::vector<uint32_t> labels (stats.size(), 0);
            connector.run (clusters, labels, stats, cluster_forming_threshold);
            if (get_cluster_sizes) {
              get_cluster_sizes->resize (stats.size());
              for (size_t i = 0; i < stats.size(); ++i)
                (*get_cluster_sizes)[i] = labels[i] ? clusters[labels[i]-1].size : 0.0;
            }
            return clusters.size() ? std::max_element (clusters.begin(), clusters.end())->size : 0.0;
          }

        protected:
          const Image::Filter::Connector& connector;
          value_type cluster_forming_threshold;
      };




      class PermutationStack {
        public:
          PermutationStack (size_t num_permutations, size_t num_samples, std::string msg, bool include_default = true) :
            num_permutations (num_permutations),
            current_permutation (0),
            progress (msg, num_permutations) {
              Math::Stats::generate_permutations (num_permutations, num_samples, permutations, include_default);
            }

          size_t next () {
            Thread::Mutex::Lock lock (permutation_mutex);
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
          Thread::Mutex permutation_mutex;
      };




      class Spatial {
        public:
          Spatial (const Image::Filter::Connector& connector, const value_type dh, const value_type E, const value_type H) :
                      connector (connector), dh (dh), E (E), H (H) {}

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>& enhanced_stats) const
          {
            enhanced_stats.resize(stats.size());
            std::fill (enhanced_stats.begin(), enhanced_stats.end(), 0.0);

            for (value_type h = this->dh; h < max_stat; h += this->dh) {
              std::vector<Image::Filter::cluster> clusters;
              std::vector<uint32_t> labels (enhanced_stats.size(), 0);
              connector.run (clusters, labels, stats, h);
              for (size_t i = 0; i < enhanced_stats.size(); ++i)
                if (labels[i])
                  enhanced_stats[i] += pow (clusters[labels[i]-1].size, this->E) * pow (h, this->H);
            }

            return *std::max_element (enhanced_stats.begin(), enhanced_stats.end());
          }

        protected:
          const Image::Filter::Connector& connector;
          const value_type dh, E, H;
      };




      class connectivity {
        public:
          connectivity () : value (0.0) { }
          value_type value;
      };




      class Connectivity {
        public:
          Connectivity (const std::vector<std::map<int32_t, connectivity> >& connectivity_map,
                        const value_type dh, const value_type E, const value_type H) :
                          connectivity_map (connectivity_map), dh (dh), E (E), H (H) { }

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>& enhanced_stats) const
          {
            enhanced_stats.resize (stats.size());
            std::fill (enhanced_stats.begin(), enhanced_stats.end(), 0.0);
            value_type max_enhanced_stat = 0.0;
            for (size_t fixel = 0; fixel < connectivity_map.size(); ++fixel) {
              std::map<int32_t, connectivity>::const_iterator connected_fixel;
              for (value_type h = this->dh; h < stats[fixel]; h +=  this->dh) {
                value_type extent = 0.0;
                for (connected_fixel = connectivity_map[fixel].begin(); connected_fixel != connectivity_map[fixel].end(); ++connected_fixel)
                  if (stats[connected_fixel->first] > h)
                    extent += connected_fixel->second.value;
                enhanced_stats[fixel] += Math::pow (extent, E) * Math::pow (h, H);
              }
              if (enhanced_stats[fixel] > max_enhanced_stat)
                max_enhanced_stat = enhanced_stats[fixel];
            }

            return max_enhanced_stat;
          }

        protected:
          const std::vector<std::map<int32_t, connectivity> >& connectivity_map;
          const value_type dh, E, H;
      };




      /*! A class to pre-compute the empirical TFCE or CFE statistic image for non-stationarity correction */
      template <class StatsType, class EnchancementType>
        class PreProcessor {
          public:
            PreProcessor (PermutationStack& permutation_stack, const StatsType& stats_calculator,
                          const EnchancementType& enhancer, std::vector<value_type>& global_enhanced_sum,
                          std::vector<size_t>& global_enhanced_count) :
                            perm_stack (permutation_stack), stats_calculator (stats_calculator),
                            enhancer (enhancer), global_enhanced_sum (global_enhanced_sum),
                            global_enhanced_count (global_enhanced_count), enhanced_sum (global_enhanced_sum.size(), 0.0),
                            enhanced_count (global_enhanced_sum.size(), 0.0), stats (global_enhanced_sum.size()),
                            enhanced_stats (global_enhanced_sum.size()) {}

            ~PreProcessor ()
            {
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
            std::vector<value_type>& global_enhanced_sum;
            std::vector<size_t>& global_enhanced_count;
            std::vector<value_type> enhanced_sum;
            std::vector<size_t> enhanced_count;
            std::vector<value_type> stats;
            std::vector<value_type> enhanced_stats;
        };




        /*! A class to perform the permutation testing */
        template <class StatsType, class EnhancementType>
          class Processor {
            public:
              Processor (PermutationStack& permutation_stack, const StatsType& stats_calculator,
                         const EnhancementType& enhancer, const RefPtr<std::vector<value_type> > empirical_enhanced_statistics,
                         const std::vector<value_type>& default_enhanced_statistics, const RefPtr<std::vector<value_type> > default_enhanced_statistics_neg,
                         Math::Vector<value_type>& perm_dist_pos, RefPtr<Math::Vector<value_type> > perm_dist_neg,
                         std::vector<size_t>& global_uncorrected_pvalue_counter, RefPtr<std::vector<size_t> > global_uncorrected_pvalue_counter_neg) :
                           perm_stack (permutation_stack), stats_calculator (stats_calculator),
                           enhancer (enhancer), empirical_enhanced_statistics (empirical_enhanced_statistics),
                           default_enhanced_statistics (default_enhanced_statistics), default_enhanced_statistics_neg (default_enhanced_statistics_neg),
                           statistics (stats_calculator.num_elements()), enhanced_statistics (stats_calculator.num_elements()),
                           uncorrected_pvalue_counter (stats_calculator.num_elements(), 0),
                           perm_dist_pos (perm_dist_pos), perm_dist_neg (perm_dist_neg),
                           global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
                           global_uncorrected_pvalue_counter_neg (global_uncorrected_pvalue_counter_neg) {
                             if (global_uncorrected_pvalue_counter_neg)
                               uncorrected_pvalue_counter_neg = new std::vector<size_t>(stats_calculator.num_elements(), 0);
              }


              ~Processor () {
                for (size_t i = 0; i < stats_calculator.num_elements(); ++i) {
                  global_uncorrected_pvalue_counter[i] += uncorrected_pvalue_counter[i];
                  if (global_uncorrected_pvalue_counter_neg)
                    (*global_uncorrected_pvalue_counter_neg)[i] = global_uncorrected_pvalue_counter[i];
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
                perm_dist_pos[index] = enhancer (max_stat, statistics, enhanced_statistics);

                if (empirical_enhanced_statistics) {
                  perm_dist_pos[index] = 0.0;
                  for (size_t i = 0; i < enhanced_statistics.size(); ++i) {
                    enhanced_statistics[i] /= (*empirical_enhanced_statistics)[i];
                    if (enhanced_statistics[i] > perm_dist_pos[index])
                      perm_dist_pos[index] = enhanced_statistics[i];
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

                  (*perm_dist_neg)[index] = enhancer (-min_stat, statistics, enhanced_statistics);

                  if (empirical_enhanced_statistics) {
                    (*perm_dist_neg)[index] = 0.0;
                    for (size_t i = 0; i < enhanced_statistics.size(); ++i) {
                      enhanced_statistics[i] /= (*empirical_enhanced_statistics)[i];
                      if (enhanced_statistics[i] > perm_dist_pos[index])
                        (*perm_dist_neg)[index] = enhanced_statistics[i];
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
              RefPtr<std::vector<value_type> > empirical_enhanced_statistics;
              const std::vector<value_type>& default_enhanced_statistics;
              const RefPtr<std::vector<value_type> > default_enhanced_statistics_neg;
              std::vector<value_type> statistics;
              std::vector<value_type> enhanced_statistics;
              std::vector<size_t> uncorrected_pvalue_counter;
              RefPtr<std::vector<size_t> > uncorrected_pvalue_counter_neg;
              Math::Vector<value_type>& perm_dist_pos;
              RefPtr<Math::Vector<value_type> > perm_dist_neg;
              std::vector<size_t>& global_uncorrected_pvalue_counter;
              RefPtr<std::vector<size_t> > global_uncorrected_pvalue_counter_neg;
        };



        template <class StatsType, class EnhancementType>
          inline void precompute_empirical_stat (const StatsType& stats_calculator, const EnhancementType& enhancer,
                                                 size_t num_permutations, std::vector<value_type>& empirical_statistic)
          {

              std::vector<size_t> global_enhanced_count (empirical_statistic.size(), 0);

              PermutationStack preprocessor_permutations (num_permutations,
                                                          stats_calculator.num_subjects(),
                                                          "precomputing empirical statistic for non-stationarity adjustment...", false);
              {
                PreProcessor<StatsType, EnhancementType> preprocessor (preprocessor_permutations, stats_calculator, enhancer,
                                                                       empirical_statistic, global_enhanced_count);
                Thread::Array< PreProcessor<StatsType, EnhancementType> > preprocessor_thread_list (preprocessor);
                Thread::Exec preprocessor_threads (preprocessor_thread_list, "preprocessor threads");
              }

              for (size_t i = 0; i < empirical_statistic.size(); ++i) {
                if (global_enhanced_count[i] > 0)
                  empirical_statistic[i] /= static_cast<value_type> (global_enhanced_count[i]);
              }
          }



          // Precompute the default statistic image and enhanced statistic. We need to precompute this for calculating the uncorrected p-values.
          template <class StatsType, class EnhancementType>
            inline void precompute_default_permutation (const StatsType& stats_calculator, const EnhancementType& enhancer,
                                                        const RefPtr<std::vector<value_type> > empirical_enhanced_statistic,
                                                        std::vector<value_type>& default_enhanced_statistics, RefPtr<std::vector<value_type> > default_enhanced_statistics_neg,
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
              }
            }




        template <class StatsType, class EnhancementType>
          inline void run_permutations (const StatsType& stats_calculator, const EnhancementType& enhancer, size_t num_permutations,
                                        const RefPtr<std::vector<value_type> > empirical_enhanced_statistic,
                                        const std::vector<value_type>& default_enhanced_statistics, const RefPtr<std::vector<value_type> > default_enhanced_statistics_neg,
                                        Math::Vector<value_type>& perm_dist_pos, RefPtr<Math::Vector<value_type> > perm_dist_neg,
                                        std::vector<value_type>& uncorrected_pvalues, RefPtr<std::vector<value_type> > uncorrected_pvalues_neg)
          {

            std::vector<size_t> global_uncorrected_pvalue_count (stats_calculator.num_elements(), 0);
            RefPtr<std::vector<size_t> > global_uncorrected_pvalue_count_neg;
            if (perm_dist_neg)
              global_uncorrected_pvalue_count_neg = new std::vector<size_t>  (stats_calculator.num_elements(), 0);

            {
              PermutationStack permutations (num_permutations,
                                             stats_calculator.num_subjects(),
                                             "running " + str(num_permutations) + " permutations...");

              Processor<StatsType, EnhancementType> processor (permutations, stats_calculator, enhancer,
                                                               empirical_enhanced_statistic,
                                                               default_enhanced_statistics, default_enhanced_statistics_neg,
                                                               perm_dist_pos, perm_dist_neg,
                                                               global_uncorrected_pvalue_count, global_uncorrected_pvalue_count_neg);
              Thread::Array< Processor<StatsType, EnhancementType> > thread_list (processor);
              Thread::Exec threads (thread_list, "permutation threads");
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
