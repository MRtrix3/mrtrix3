/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 23/07/11.

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

#ifndef __stats_permute_h__
#define __stats_permute_h__

namespace MR
{
  namespace Stats
  {

    class Item
    {
      public:
        std::vector<size_t> labelling;
        size_t index;
    };


    typedef Thread::Queue<Item> Queue;
    typedef float value_type;

    class DataLoader
    {
      public:
        DataLoader (size_t num_perms, size_t num_subjects) :
          num_perms (num_perms),
          num_subjects (num_subjects),
          current_perm (0),
          progress ("running " + str(num_perms) + " permutations...", num_perms) { }

        bool operator() (Item& item) {
          if (current_perm >= num_perms) 
            return false;

          item.index = current_perm;
          for (size_t i = 0; i < num_subjects; ++i)
            item.labelling.push_back(i);

          if (current_perm) {
            do {
              std::random_shuffle (item.labelling.begin(), item.labelling.end());
            } while (is_duplicate_permutation (item.labelling, previous_perms));
          }
          previous_perms.push_back (item.labelling);
          ++current_perm;
          ++progress;

          return true;
        }

      private:

        bool is_duplicate_vector (std::vector<size_t> & v1, std::vector<size_t> & v2) {
          for (size_t i = 0; i < v1.size(); i++) {
            if (v1[i] != v2[i])
              return false;
          }
          return true;
        }

        bool is_duplicate_permutation (std::vector<size_t> & perm, std::vector<std::vector<size_t> > & previous_permutations) {
          for (unsigned int p = 0; p < previous_permutations.size(); p++) {
            if (is_duplicate_vector (perm, previous_permutations[p]))
              return true;
          }
          return false;
        }

        std::vector <std::vector<size_t> > previous_perms;
        size_t num_perms, num_subjects, current_perm;
        ProgressBar progress;
    };









    class Processor
    {
      public:
        Processor (Ptr<Image::Filter::Connector<Image::Buffer<value_type>::voxel_type> > connector,
                   Math::Vector<value_type>& perm_distribution_pos, Math::Vector<value_type>& perm_distribution_neg,
                   const Math::Matrix<value_type>& afd,
                   std::vector<value_type>& tfce_output_pos, std::vector<value_type>& tfce_output_neg,
                   const Math::Matrix<value_type>& design_matrix, const Math::Matrix<value_type>& contrast_matrix,
                   value_type dh, value_type E, value_type H) :
          connector(connector),
          perm_distribution_pos (perm_distribution_pos),
          perm_distribution_neg (perm_distribution_neg),
          afd (afd),
          tfce_output_pos (tfce_output_pos),
          tfce_output_neg (tfce_output_neg),
          design_matrix (design_matrix), 
          contrast_matrix (contrast_matrix),
          dh (dh),
          E (E),
          H (H) { }

        bool operator() (const Item& item) {
          std::vector<value_type> stats (afd.rows(), 0.0);
          std::vector<value_type> tfce_stats (afd.rows(), 0.0);
          value_type max_stat = 0.0, min_stat = 0.0;

          compute_tstatistics (item.labelling, stats, max_stat, min_stat);
          value_type max_tfce_stat = tfce_integration (max_stat, stats, tfce_stats);
          if (item.index == 0)
            tfce_output_pos = tfce_stats;
          else
            perm_distribution_pos[item.index - 1] = max_tfce_stat;

          for (size_t i = 0; i < afd.rows(); ++i) {
            stats[i] = -stats[i];
            tfce_stats[i] = 0.0;
          }
          max_tfce_stat = tfce_integration (-min_stat, stats, tfce_stats);
          if (item.index == 0)
            tfce_output_neg = tfce_stats;
          else
            perm_distribution_neg[item.index - 1] = max_tfce_stat;

          return true;
        }

      private:

        value_type tfce_integration (const value_type max_stat, const std::vector<value_type>& stats, std::vector<value_type>& tfce_stats) 
        {
          for (value_type threshold = dh; threshold < max_stat; threshold += dh) {
            std::vector<Image::Filter::cluster> clusters;
            std::vector<uint32_t> labels (afd.rows(), 0);
            connector->run (clusters, labels, stats, threshold);

            for (size_t i = 0; i < afd.rows(); ++i) 
              if (labels[i])
                tfce_stats[i] += pow (clusters[labels[i] - 1].size, E) * pow (threshold, H);
          }

          value_type max_tfce_stat = 0.0;
          for (size_t i = 0; i < afd.rows(); i++) 
            if (tfce_stats[i] > max_tfce_stat)
              max_tfce_stat = tfce_stats[i];
          
          return max_tfce_stat;
        }

        // Compute the test statistic along each voxel/direction
        void compute_tstatistics (const std::vector<size_t>& perms, std::vector<value_type>& stats, value_type& max_stat, value_type& min_stat) 
        {
          Math::Matrix<value_type> design (design_matrix);

          for (size_t i = 0; i < design.rows(); ++i)
            design(i,0) = design_matrix (perms[i], 0);

          Math::mult (Mt_M, value_type(1.0), CblasTrans, design, CblasNoTrans, design);
          Math::LU::inv (inv_Mt_M, Mt_M);
          Math::mult (pinv_M, value_type(1.0), CblasNoTrans, inv_Mt_M, CblasTrans, design);

          for (size_t i = 0; i < afd.rows(); ++i) {
            stats[i] = compute_tstatistic (afd.row(i), design);
            if (stats[i] > max_stat)
              max_stat = stats[i];
            if (stats[i] < min_stat)
              min_stat = stats[i];
          }
        }


        value_type compute_tstatistic (const Math::Vector<value_type>& values, const Math::Matrix<value_type>& design) 
        {
          Math::mult (beta, pinv_M, values);
          value_type contrast_dot_beta = Math::dot (contrast_matrix.row(0), beta);

          Math::mult (residuals, design, beta);
          Math::mult (beta, inv_Mt_M, contrast_matrix.row(0));
          value_type var_E = Math::norm2 (residuals) * Math::dot (contrast_matrix.row(0), beta);

          return contrast_dot_beta / Math::sqrt (var_E);
        }

        Ptr<Image::Filter::Connector<Image::Buffer<value_type>::voxel_type> > connector;
        Math::Vector<value_type>& perm_distribution_pos, perm_distribution_neg;
        const Math::Matrix<value_type>& afd;
        std::vector<value_type>& tfce_output_pos, tfce_output_neg;
        const Math::Matrix<value_type>& design_matrix;
        const Math::Matrix<value_type>& contrast_matrix;
        value_type dh, E, H;
        Math::Matrix<value_type> Mt_M, inv_Mt_M, pinv_M;
        Math::Vector<value_type> beta, residuals;
    };







    template <class StatVoxelType, class PvalueVoxelType>
      void statistic2pvalue (Math::Vector<value_type> & perm_dist, StatVoxelType stat_voxel, PvalueVoxelType p_voxel) {

        std::vector <value_type> permutations (perm_dist.size(), 0);
        for (size_t i = 0; i < perm_dist.size(); i++)
          permutations[i] = perm_dist[i];
        std::sort (permutations.begin(), permutations.end());

        Image::LoopInOrder outer (p_voxel);
        for (outer.start (p_voxel, stat_voxel); outer.ok(); outer.next (p_voxel, stat_voxel)) {
          value_type tvalue = stat_voxel.value();
          if (tvalue > 0.0) {
            value_type pvalue = 1.0;
            for (size_t i = 0; i < permutations.size(); ++i) {
              if (tvalue < permutations[i]) {
                pvalue = value_type(i) / value_type(permutations.size());
                break;
              }
            }
            p_voxel.value() = pvalue;
          } 
          else 
            p_voxel.value() = 0.0;
        }
      }

  }
}


#endif
