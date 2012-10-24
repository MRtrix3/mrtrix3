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
#ifndef __math_stats_permutation_h__
#define __math_stats_permutation_h__

#include "math/vector.h"
#include "math/matrix.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {

      inline bool is_duplicate_vector (const std::vector<size_t>& v1, const std::vector<size_t>& v2)
      {
        for (size_t i = 0; i < v1.size(); i++) {
          if (v1[i] != v2[i])
            return false;
        }
        return true;
      }

      inline bool is_duplicate_permutation (const std::vector<size_t>& perm, const std::vector<std::vector<size_t> >& previous_permutations)
      {
        for (unsigned int p = 0; p < previous_permutations.size(); p++) {
          if (is_duplicate_vector (perm, previous_permutations[p]))
            return true;
        }
        return false;
      }

      inline void generate_permutations (const size_t num_perms, const size_t num_subjects, std::vector<std::vector<size_t> >& permutations)
      {
        permutations.clear();
        std::vector<size_t> default_labelling (num_subjects);
        for (size_t i = 0; i < num_subjects; ++i)
          default_labelling.push_back(i);
        permutations.push_back (default_labelling);
        for (size_t p = 0; p  < num_perms; ++p) {
          std::vector<size_t> permuted_labelling (default_labelling);
          do {
            std::random_shuffle (permuted_labelling.begin(), permuted_labelling.end());
          } while (is_duplicate_permutation (permuted_labelling, permutations));
          permutations.push_back(permuted_labelling);
        }
      }

      typedef float value_type;

      template <class StatVoxelType, class PvalueVoxelType>
        void statistic2pvalue (const Math::Vector<value_type>& perm_dist, StatVoxelType stat_voxel, PvalueVoxelType p_voxel)
        {
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
}

#endif
