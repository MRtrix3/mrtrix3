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
        std::vector<unsigned int> labelling;
        unsigned int index;
    };


    typedef Thread::Queue<Item> Queue;

    class DataLoader
    {
      public:
        DataLoader (unsigned int num_perms,
                    unsigned int num_subjects) :
                    num_perms_ (num_perms),
                    num_subjects_ (num_subjects),
                    current_perm_ (0) { }

        bool operator() (Item& item) {
          if (current_perm_ < num_perms_) {
            item.index = current_perm_;
            for (size_t i = 0; i < num_subjects_; ++i)
              item.labelling.push_back(i);
            if (current_perm_) {
              do {
                random_shuffle(item.labelling.begin(), item.labelling.end());
              } while (is_duplicate_permutation (item.labelling, previous_perms_));
            }
            previous_perms_.push_back(item.labelling);
            current_perm_++;
            return true;
          }
          return false;
        }

      private:

        bool is_duplicate_vector (std::vector<unsigned int> & v1, std::vector<unsigned int> & v2) {
          for (unsigned int i = 0; i < v1.size(); i++) {
            if (v1[i] != v2[i])
              return false;
          }
          return true;
        }

        bool is_duplicate_permutation (std::vector<unsigned int> & perm, std::vector<std::vector<unsigned int> > & previous_permutations) {
          for (unsigned int p = 0; p < previous_permutations.size(); p++) {
            if (is_duplicate_vector (perm, previous_permutations[p]))
              return true;
          }
          return false;
        }

        std::vector <std::vector<unsigned int> > previous_perms_;
        unsigned int num_perms_;
        unsigned int num_subjects_;
        unsigned int current_perm_;
    };



    class Processor
    {
      public:
        Processor (Ptr<Image::Filter::Connector<Image::Buffer<float>::voxel_type> > connector,
                   Math::Vector<float> & perm_distribution_1,
                   Math::Vector<float> & perm_distribution_2,
                   std::vector<std::vector<float> > & afd,
                   std::vector<float> & tfce_output_1,
                   std::vector<float> & tfce_output_2,
                   unsigned int num1,
                   float dh,
                   float E,
                   float H,
                   ProgressBar & progress) :
                   connector_(connector),
                   perm_distribution_1_ (perm_distribution_1),
                   perm_distribution_2_ (perm_distribution_2),
                   afd_ (afd),
                   tfce_output_1_ (tfce_output_1),
                   tfce_output_2_ (tfce_output_2),
                   num_vox_dir_ (afd.size()),
                   num1_ (num1),
                   dh_ (dh),
                   E_(E),
                   H_(H),
                   progress_ (progress) { }

        bool operator() (const Item& item) {
          std::vector<float> stats (num_vox_dir_, 0);
          std::vector<float> tfce_stats (num_vox_dir_, 0);
          float max_stat = 0;
          float min_stat = 0;
          compute_tstatistics (item.labelling, stats, max_stat, min_stat);
          float max_tfce_stat = tfce_integration (max_stat, stats, tfce_stats);
          if (item.index == 0)
            tfce_output_1_ = tfce_stats;
          else
            perm_distribution_1_[item.index - 1] = max_tfce_stat;

          for (size_t i = 0; i < num_vox_dir_; ++i) {
            stats[i] = -stats[i];
            tfce_stats[i] = 0.0;
          }
          max_tfce_stat = tfce_integration (-min_stat, stats, tfce_stats);
          if (item.index == 0)
            tfce_output_2_ = tfce_stats;
          else
            perm_distribution_2_[item.index - 1] = max_tfce_stat;

          progress_++;
          return true;
        }

      private:

        float tfce_integration (const float max_stat, const std::vector<float>& stats, std::vector<float>& tfce_stats) {
          for (float threshold = dh_; threshold < max_stat; threshold += dh_) {
            std::vector <Image::Filter::cluster> clusters;
            std::vector<uint32_t> labels (num_vox_dir_, 0);
            connector_->run (clusters, labels, stats, threshold);
            for (uint32_t v = 0; v < num_vox_dir_; v++) {
              if (labels[v])
                tfce_stats[v] += pow (clusters[labels[v] - 1].size, E_) * pow (threshold, H_);
            }
          }

          float max_tfce_stat = 0;
          for (uint v = 0; v < num_vox_dir_; v++) {
            if (tfce_stats[v] > max_tfce_stat)
              max_tfce_stat = tfce_stats[v];
          }
          return max_tfce_stat;
        }

        // Compute the test statistic along each voxel/direction
        void compute_tstatistics (const std::vector<unsigned int>& perms, std::vector<float>& stats, float& max_stat, float& min_stat) {
          float fnum1 = static_cast<float> (num1_);
          float fnum2 = static_cast<float> (perms.size() - num1_);

          for (uint32_t v = 0; v < afd_.size(); v++) {

            float mean1 = 0;
            for (size_t i = 0; i < num1_; i++){
              mean1 += afd_[v][perms[i]];
            }
            mean1 /= fnum1;

            float mean2 = 0;
            for (unsigned int i = num1_; i < perms.size(); i++){
              mean2 += afd_[v][perms[i]];
            }
            mean2 /= fnum2;

            float var1 = 0;
            for (size_t i = 0; i < num1_; i++){
              var1 += (afd_[v][perms[i]] - mean1) *  (afd_[v][perms[i]] - mean1);
            }
            var1 = var1 / (fnum1 - 1.0);

            float var2 = 0;
            for (unsigned int i = num1_; i < perms.size(); i++){
              var2 += (afd_[v][perms[i]] - mean2) *  (afd_[v][perms[i]] - mean2);
            }
            var2 = var2 / (fnum2 - 1.0);

            float df = fnum1 + fnum2 - 2.0;
            float sp = sqrt(((fnum1-1.0) * var1 + (fnum2-1.0) * var2) / df);
            stats[v] = (mean2 - mean1) / (sp * sqrt(1.0 / fnum2 + 1.0 / fnum1));
            if (stats[v] > max_stat)
              max_stat = stats[v];
            if (stats[v] < min_stat)
              min_stat = stats[v];
          }
        }

        Ptr<Image::Filter::Connector<Image::Buffer<float>::voxel_type> > connector_;
        Math::Vector<float> & perm_distribution_1_;
        Math::Vector<float> & perm_distribution_2_;
        std::vector<std::vector<float> > & afd_;
        std::vector<float> & tfce_output_1_;
        std::vector<float> & tfce_output_2_;
        size_t num_vox_dir_;
        unsigned int num1_;
        float dh_;
        float E_;
        float H_;
        ProgressBar & progress_;
    };


    template <class StatVoxelType, class PvalueVoxelType>
    void statistic2pvalue (Math::Vector<float> & perm_dist, StatVoxelType stat_voxel, PvalueVoxelType p_voxel) {

      std::vector <float> permutations (perm_dist.size(), 0);
      for (uint i = 0; i < perm_dist.size(); i++)
        permutations[i] = perm_dist[i];
      std::sort(permutations.begin(), permutations.end());

      Image::LoopInOrder outer (p_voxel);
      for (outer.start (p_voxel, stat_voxel); outer.ok(); outer.next (p_voxel, stat_voxel)) {
        float tvalue = stat_voxel.value();
        if (tvalue > 0) {
          uint i = 0;
          while (i < permutations.size()) {
            if (tvalue > permutations[i]) {
              i++;
              continue;
            } else {
              break;
            }
          }
          if (i == permutations.size()) {
            p_voxel.value() = 1.0;
          } else {
            p_voxel.value() = float(i) / float(permutations.size());
          }
        } else {
          p_voxel.value() = 0;
        }
      }
    }

  }
}


#endif
