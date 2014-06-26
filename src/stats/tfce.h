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
                                 std::vector<value_type>* get_tfce_stats)
          {
            tfce_stats.resize(stats.size());
            std::fill (tfce_stats.begin(), tfce_stats.end(), 0.0);

            for (value_type h = this->dh; h < max_stat; h += this->dh) {
              std::vector<Image::Filter::cluster> clusters;
              std::vector<uint32_t> labels (tfce_stats.size(), 0);
              connector.run (clusters, labels, stats, h);
              for (size_t i = 0; i < tfce_stats.size(); ++i)
                if (labels[i])
                  tfce_stats[i] += pow (clusters[labels[i]-1].size, this->E) * pow (h, this->H);
            }

            if (get_tfce_stats)
              *get_tfce_stats = tfce_stats;

            return *std::max_element (tfce_stats.begin(), tfce_stats.end());
          }

        protected:
          const Image::Filter::Connector& connector;
          std::vector<value_type> tfce_stats;
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
                                 std::vector<value_type>* get_tfce_stats)
          {
            tfce_stats.resize (stats.size());
            std::fill (tfce_stats.begin(), tfce_stats.end(), 0.0);
            value_type max_tfce_stat = 0.0;
            for (size_t fixel = 0; fixel < connectivity_map.size(); ++fixel) {
              std::map<int32_t, connectivity>::const_iterator connected_fixel;
              for (value_type h = this->dh; h < stats[fixel]; h +=  this->dh) {
                value_type extent = 0.0;
                for (connected_fixel = connectivity_map[fixel].begin(); connected_fixel != connectivity_map[fixel].end(); ++connected_fixel)
                  if (stats[connected_fixel->first] > h)
                    extent += connected_fixel->second.value;
                tfce_stats[fixel] += Math::pow (extent, E) * Math::pow (h, H);
              }
              if (tfce_stats[fixel] > max_tfce_stat)
                max_tfce_stat = tfce_stats[fixel];
            }

            if (get_tfce_stats) 
              *get_tfce_stats = tfce_stats;

            return max_tfce_stat;
          }

        protected:
          const std::vector<std::map<int32_t, connectivity> >& connectivity_map;
          std::vector<value_type> tfce_stats;
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




      template <class StatsType, class TFCEType>
        class Processor {
          public:
            Processor (PermutationStack& permutation_stack, const StatsType& stats_calculator,
                       const TFCEType& tfce_integrator, Math::Vector<value_type>& perm_distribution_pos,
                       Math::Vector<value_type>& perm_distribution_neg, std::vector<value_type>& tfce_output_pos,
                       std::vector<value_type>& tfce_output_neg, std::vector<value_type>& tvalue_output) :
                         perm_stack (permutation_stack), stats_calculator (stats_calculator),
                         tfce_integrator (tfce_integrator), perm_distribution_pos (perm_distribution_pos),
                         perm_distribution_neg (perm_distribution_neg), tfce_output_pos (tfce_output_pos),
                         tfce_output_neg (tfce_output_neg), tvalue_output (tvalue_output) {
              }


            void execute () 
            { 
              size_t index;
              while (( index = perm_stack.next() ) < perm_stack.num_permutations) 
                process_permutation (index);
              
            }

          protected:
            PermutationStack& perm_stack;
            StatsType stats_calculator;
            TFCEType tfce_integrator;
            Math::Vector<value_type>& perm_distribution_pos;
            Math::Vector<value_type>& perm_distribution_neg;
            std::vector<value_type>& tfce_output_pos;
            std::vector<value_type>& tfce_output_neg;
            std::vector<value_type>& tvalue_output;

            void process_permutation (size_t index)
            {
              value_type max_stat = 0.0, min_stat = 0.0;
              std::vector<value_type> stats;
              stats_calculator (perm_stack.permutation (index), stats, max_stat, min_stat);

              if (index == 0)
                tvalue_output = stats;

              value_type max_tfce_stat = tfce_integrator (max_stat, stats, index ? NULL : &tfce_output_pos);
              if (index)
                perm_distribution_pos[index-1] = max_tfce_stat;

              for (size_t i = 0; i < stats.size(); ++i) 
                stats[i] = -stats[i];

              max_tfce_stat = tfce_integrator (-min_stat, stats, index ? NULL : &tfce_output_neg);
              if (index)
                perm_distribution_neg[index-1] = max_tfce_stat;
            }

        };




      template <class StatsType, class TFCEType> 
        inline void run (const StatsType& stats_calculator, const TFCEType& tfce_integrator, size_t num_permutations, 
            Math::Vector<value_type>& perm_distribution_pos, Math::Vector<value_type>& perm_distribution_neg,
            std::vector<value_type>& tfce_output_pos, std::vector<value_type>& tfce_output_neg, std::vector<value_type>& tvalue_output) 
        {
          PermutationStack permutation_stack (num_permutations, stats_calculator.num_samples());
          Processor<StatsType, TFCEType> processor (permutation_stack, stats_calculator, tfce_integrator, perm_distribution_pos, perm_distribution_neg, tfce_output_pos, tfce_output_neg, tvalue_output);
          Thread::Array< Processor<StatsType, TFCEType> > thread_list (processor);
          Thread::Exec threads (thread_list, "TFCE threads");
        }

    }
  }
}

#endif
