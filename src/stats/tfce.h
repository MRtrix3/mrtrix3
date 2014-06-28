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



      class Spatial {
        public:
          Spatial (const Image::Filter::Connector& connector, value_type dh, value_type E, value_type H) :
                      connector (connector), dh (dh), E (E), H (H) {}

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>* get_enhanced_stats)
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

            if (get_enhanced_stats)
              *get_enhanced_stats = enhanced_stats;

            return *std::max_element (enhanced_stats.begin(), enhanced_stats.end());
          }

        protected:
          const Image::Filter::Connector& connector;
          std::vector<value_type> enhanced_stats;
          value_type dh, E, H;
      };



      class connectivity {
        public:
          connectivity () : value (0.0) { }
          value_type value;
      };



      class Connectivity {
        public:
          Connectivity (const std::vector<std::map<int32_t, connectivity> >& connectivity_map,
                        value_type dh, value_type E, value_type H) :
                          connectivity_map (connectivity_map), dh (dh), E (E), H (H) { }

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>* get_enhanced_stats)
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

            if (get_enhanced_stats)
              *get_enhanced_stats = enhanced_stats;

            return max_enhanced_stat;
          }

        protected:
          const std::vector<std::map<int32_t, connectivity> >& connectivity_map;
          std::vector<value_type> enhanced_stats;
          value_type dh, E, H;
      };



      class PermutationStack {
        public:
          PermutationStack (size_t num_permutations, size_t num_samples) :
            num_permutations (num_permutations),
            current_permutation (0),
            progress ("running " + str(num_permutations) + " permutations...", num_permutations) {
              Math::Stats::generate_permutations (num_permutations, num_samples, permutations);
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
                            enhanced_count (global_enhanced_sum.size(), 0.0) {
            }

            ~PreProcessor () {
              for (size_t i = 0; i < global_enhanced_sum.size(); ++i) {
                global_enhanced_sum[i] += enhanced_sum[i];
                global_enhanced_count[i] += enhanced_count[i];
              }
            }

            void execute () {
              size_t index;
              while (( index = perm_stack.next() ) < perm_stack.num_permutations)
                process_permutation (index);
            }

          protected:

            void process_permutation (size_t index) {
              value_type max_stat = 0.0, min_stat = 0.0;
              std::vector<value_type> stats;
              stats_calculator (perm_stack.permutation (index), stats, max_stat, min_stat);
              std::vector<value_type> enhanced_stats;
              enhancer (max_stat, stats, &enhanced_stats);

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
        };



        /*! A class to perform the permutation testing */
        template <class StatsType, class EnhancementType>
          class Processor {
            public:
              Processor (PermutationStack& permutation_stack, const StatsType& stats_calculator,
                         const EnhancementType& enhancer, const bool do_nonstationarity,
                         const std::vector<value_type>& empirical_enhanced_statistic,
                         Math::Vector<value_type>& perm_dist_pos, Math::Vector<value_type>& perm_dist_neg,
                         std::vector<value_type>& enhanced_output_pos, std::vector<value_type>& enhanced_output_neg,
                         std::vector<value_type>& tvalue_output) :
                           perm_stack (permutation_stack), stats_calculator (stats_calculator),
                           enhancer (enhancer), do_nonstationarity (do_nonstationarity),
                           empirical_enhanced_statistic (empirical_enhanced_statistic), enhanced_statistic (0),
                           perm_dist_pos (perm_dist_pos), perm_dist_neg (perm_dist_neg),
                           enhanced_output_pos (enhanced_output_pos), enchanced_output_neg (enhanced_output_neg), tvalue_output (tvalue_output) {
                  if (do_nonstationarity)
                    enhanced_statistic.resize (empirical_enhanced_statistic.size());
                }

              void execute () {
                size_t index;
                while (( index = perm_stack.next() ) < perm_stack.num_permutations)
                  process_permutation (index);
              }


            protected:
              value_type nonstationarity_enhancement (const value_type max_stat,
                                                      const std::vector<value_type>& stats,
                                                      std::vector<value_type>* get_enhanced_stats) {
                enhancer (max_stat, stats, &enhanced_statistic);
                value_type max_enhanced_statistic = 0.0;
                for (size_t i = 0; i < stats.size(); ++i) {
                  enhanced_statistic[i] /= empirical_enhanced_statistic[i];
                  if (enhanced_statistic[i] > max_enhanced_statistic)
                    max_enhanced_statistic = enhanced_statistic[i];
                }
                if (get_enhanced_stats)
                  *get_enhanced_stats = enhanced_statistic;
                return max_enhanced_statistic;
              }

              void process_permutation (size_t index) {
                value_type max_stat = 0.0, min_stat = 0.0;
                std::vector<value_type> stats;
                stats_calculator (perm_stack.permutation (index), stats, max_stat, min_stat);
                if (index == 0)
                  tvalue_output = stats;
                value_type max_enhanced_statistic = 0.0;
                if (do_nonstationarity)
                  max_enhanced_statistic = nonstationarity_enhancement (max_stat, stats, index ? NULL : &enhanced_output_pos);
                else
                  max_enhanced_statistic = enhancer (max_stat, stats, index ? NULL : &enhanced_output_pos);
                if (index)
                  perm_dist_pos[index-1] = max_enhanced_statistic;

                // Compute the opposite contrast
                for (size_t i = 0; i < stats.size(); ++i)
                  stats[i] = -stats[i];
                if (do_nonstationarity)
                  max_enhanced_statistic = nonstationarity_enhancement (-max_stat, stats, index ? NULL : &enhanced_output_pos);
                else
                  max_enhanced_statistic = enhancer (-min_stat, stats, index ? NULL : &enchanced_output_neg);
                if (index)
                  perm_dist_neg[index-1] = max_enhanced_statistic;
              }

              PermutationStack& perm_stack;
              StatsType stats_calculator;
              EnhancementType enhancer;
              const bool do_nonstationarity;
              const std::vector<value_type> empirical_enhanced_statistic;
              std::vector<value_type> enhanced_statistic;
              std::vector<value_type> adjusted_enhanced_statistic;
              Math::Vector<value_type>& perm_dist_pos;
              Math::Vector<value_type>& perm_dist_neg;
              std::vector<value_type>& enhanced_output_pos;
              std::vector<value_type>& enchanced_output_neg;
              std::vector<value_type>& tvalue_output;
        };



        template <class StatsType, class EnhancementType>
          inline void precompute_empirical_enhanced_statistic (const StatsType& stats_calculator, const EnhancementType& enhancer, size_t num_permutations,
                                                               std::vector<value_type>& empirical_enhanced_statistic)
          {
            PermutationStack permutation_stack (num_permutations, stats_calculator.num_samples());
            std::vector<size_t> global_enhanced_count (stats_calculator.num_samples(), 0.0);
            PreProcessor<StatsType, EnhancementType> processor (permutation_stack, stats_calculator, enhancer,
                                                                empirical_enhanced_statistic, global_enhanced_count);
            Thread::Array< PreProcessor<StatsType, EnhancementType> > thread_list (processor);
            Thread::Exec threads (thread_list, "enhanced threads");
            for (size_t i = 0; i < stats_calculator.num_samples(); ++i) {
              empirical_enhanced_statistic[i] /= static_cast<value_type> (global_enhanced_count[i]);
              if (empirical_enhanced_statistic[i] < 0.0) {
                empirical_enhanced_statistic[i] = 1.0;
                WARN ("emprical enhanced statistic below 1.0");
              }
            }
          }




        template <class StatsType, class EnhancementType>
          inline void run (const StatsType& stats_calculator, const EnhancementType& enhancer, size_t num_permutations,
                           const bool do_nonstationary, const std::vector<value_type> empirical_enchanced_statistic,
                           Math::Vector<value_type>& perm_dist_pos, Math::Vector<value_type>& perm_dist_neg,
                           std::vector<value_type>& enhanced_output_pos, std::vector<value_type>& enhanced_output_neg, std::vector<value_type>& tvalue_output)
          {
            PermutationStack permutation_stack (num_permutations, stats_calculator.num_samples());
            Processor<StatsType, EnhancementType> processor (permutation_stack, stats_calculator, enhancer,
                                                             do_nonstationary, empirical_enchanced_statistic,
                                                             perm_dist_pos, perm_dist_neg, enhanced_output_pos,
                                                             enhanced_output_neg, tvalue_output);
            Thread::Array< Processor<StatsType, EnhancementType> > thread_list (processor);
            Thread::Exec threads (thread_list, "enhanced threads");
          }
          //! @}

    }
  }
}

#endif
