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

      class PermutationItem
      {
        public:
          std::vector<size_t> labelling;
          size_t index;
      };

      typedef Thread::Queue<PermutationItem> Queue;
      typedef float value_type;

      class PermutationQueueLoader
      {
       public:
         PermutationQueueLoader (size_t num_perms, size_t num_subjects) :
           current_perm (0),
           progress ("running " + str(num_perms) + " permutations...", num_perms) {
           Math::Stats::generate_permutations (num_perms, num_subjects, permutations);
         }

         bool operator() (PermutationItem& item) {
           if (current_perm >= permutations.size())
             return false;

           item.index = current_perm;
           item.labelling = permutations[current_perm];
           ++current_perm;
           ++progress;

           return true;
         }

       private:

         std::vector <std::vector<size_t> > permutations;
         size_t num_perms, num_subjects, current_perm;
         ProgressBar progress;
      };



      template <class StatsType>
        class Base {
          public:
            Base (StatsType& stats_calculator,
                  Math::Vector<value_type>& perm_distribution_pos, Math::Vector<value_type>& perm_distribution_neg,
                  value_type dh, value_type E, value_type H,
                  std::vector<value_type>& tfce_output_pos, std::vector<value_type>& tfce_output_neg,
                  std::vector<value_type>& tvalue_output) :
                    stats_calculator (stats_calculator),
                    perm_distribution_pos (perm_distribution_pos), perm_distribution_neg (perm_distribution_neg),
                    dh (dh), E (E), H (H),
                    tfce_output_pos (tfce_output_pos), tfce_output_neg (tfce_output_neg), tvalue_output (tvalue_output) {
          }

          protected:
            StatsType& stats_calculator;
            Math::Vector<value_type>& perm_distribution_pos,  perm_distribution_neg;
            value_type dh, E, H;
            std::vector<value_type>& tfce_output_pos, tfce_output_neg, tvalue_output;

      };

      template <class StatsType>
      class SpatialCluster : public Base<StatsType>
      {
        public:
          SpatialCluster (StatsType& stats_calculator,
                          Math::Vector<value_type>& perm_distribution_pos, Math::Vector<value_type>& perm_distribution_neg,
                          value_type dh, value_type E, value_type H,
                          std::vector<value_type>& tfce_output_pos, std::vector<value_type>& tfce_output_neg,
                          std::vector<value_type>& tvalue_output,
                          const Image::Filter::Connector& connector) :
                            Base<StatsType> (stats_calculator, perm_distribution_pos, perm_distribution_neg,
                            dh, E, H, tfce_output_pos, tfce_output_neg, tvalue_output), connector (connector) {}

          bool operator() (const PermutationItem& item)
          {
            value_type max_stat = 0.0, min_stat = 0.0;

            std::vector<value_type> stats;
            this->stats_calculator (item.labelling, stats, max_stat, min_stat);
            if (item.index == 0)
              this->tvalue_output = stats;

            std::vector<value_type> tfce_stats (stats.size(), 0.0);
            value_type max_tfce_stat = tfce_integration (max_stat, stats, tfce_stats);
            if (item.index == 0)
              this->tfce_output_pos = tfce_stats;
            else
              this->perm_distribution_pos[item.index - 1] = max_tfce_stat;

            for (size_t i = 0; i < stats.size(); ++i) {
              stats[i] = -stats[i];
              tfce_stats[i] = 0.0;
            }
            max_tfce_stat = tfce_integration (-min_stat, stats, tfce_stats);
            if (item.index == 0)
              this->tfce_output_neg = tfce_stats;
            else
              this->perm_distribution_neg[item.index - 1] = max_tfce_stat;

            return true;
          }

        private:

          value_type tfce_integration (const value_type max_stat, const std::vector<value_type>& stats, std::vector<value_type>& tfce_stats)
          {
            for (value_type threshold = this->dh; threshold < max_stat; threshold += this->dh) {
              std::vector<Image::Filter::cluster> clusters;
              std::vector<uint32_t> labels (tfce_stats.size(), 0);
              connector.run (clusters, labels, stats, threshold);

              for (size_t i = 0; i < tfce_stats.size(); ++i)
                if (labels[i])
                  tfce_stats[i] += pow (clusters[labels[i]-1].size, this->E) * pow (threshold, this->H);
            }

            value_type max_tfce_stat = 0.0;
            for (size_t i = 0; i < tfce_stats.size(); i++)
              if (tfce_stats[i] > max_tfce_stat)
                max_tfce_stat = tfce_stats[i];

            return max_tfce_stat;
          }

          const Image::Filter::Connector& connector;
      };


      class LobeItem
      {
        public:
          size_t index;
      };


      struct connectivity
      {
          value_type value;
          connectivity(): value (0.0) {}
      };

      template <class StatsType>
        class ConnectivityCluster : public Base<StatsType>
        {
         public:
          ConnectivityCluster (StatsType& stats_calculator,
                               Math::Vector<value_type>& perm_distribution_pos, Math::Vector<value_type>& perm_distribution_neg,
                               value_type dh, value_type E, value_type H,
                               std::vector<value_type>& tfce_output_pos, std::vector<value_type>& tfce_output_neg,
                               std::vector<value_type>& tvalue_output,
                               std::vector<std::map<int32_t, connectivity> >& connectivity_map) :
                                 Base<StatsType> (stats_calculator, perm_distribution_pos, perm_distribution_neg,
                                 dh, E, H, tfce_output_pos, tfce_output_neg, tvalue_output), connectivity_map (connectivity_map) {}

          bool operator() (const LobeItem& item)
          {
            //HERE
            return true;
          }

        private:

          value_type tfce_integration (const value_type max_stat, const std::vector<value_type>& stats, std::vector<value_type>& tfce_stats)
          {
            for (value_type threshold = this->dh; threshold < max_stat; threshold +=  this->dh) {
              for (size_t lobe = 0; lobe < connectivity_map.size(); ++lobe) {
                float extent = 0.0;
                std::map<int32_t, connectivity>::iterator connected_lobe = connectivity_map[lobe].begin();
                for (;connected_lobe != connectivity_map[lobe].end(); ++connected_lobe)
                  if (stats[connected_lobe->first] > threshold)
                    extent += connected_lobe->second.value;
                tfce_stats[lobe] += pow (extent,  this->E) * pow (threshold,  this->H);
              }
            }

            value_type max_tfce_stat = 0.0;
            for (size_t i = 0; i < stats.size(); i++)
              if (tfce_stats[i] > max_tfce_stat)
                max_tfce_stat = tfce_stats[i];

            return max_tfce_stat;
          }

          std::vector<std::map<int32_t, connectivity> >& connectivity_map;
      };
    }
  }
}

#endif
