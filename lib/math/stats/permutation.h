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
#ifndef __math_stats_permutation_h__
#define __math_stats_permutation_h__

namespace MR
{
  namespace Math
  {
    namespace Stats
    {

      typedef float value_type;


      inline bool is_duplicate_vector (const vector<size_t>& v1, const vector<size_t>& v2)
      {
        for (size_t i = 0; i < v1.size(); i++) {
          if (v1[i] != v2[i])
            return false;
        }
        return true;
      }


      inline bool is_duplicate_permutation (const vector<size_t>& perm,
                                            const vector<vector<size_t> >& previous_permutations)
      {
        for (unsigned int p = 0; p < previous_permutations.size(); p++) {
          if (is_duplicate_vector (perm, previous_permutations[p]))
            return true;
        }
        return false;
      }

      // Note that this function does not take into account grouping of subjects and therefore generated
      // permutations are not guaranteed to be unique wrt the computed test statistic.
      // If the number of subjects is large then the likelihood of generating duplicates is low.
      inline void generate_permutations (const size_t num_perms,
                                         const size_t num_subjects,
                                         vector<vector<size_t> >& permutations,
                                         bool include_default)
      {
        permutations.clear();
        vector<size_t> default_labelling (num_subjects);
        for (size_t i = 0; i < num_subjects; ++i)
          default_labelling[i] = i;
        size_t p = 0;
        if (include_default) {
          permutations.push_back (default_labelling);
          ++p;
        }
        for (;p < num_perms; ++p) {
          vector<size_t> permuted_labelling (default_labelling);
          do {
            std::random_shuffle (permuted_labelling.begin(), permuted_labelling.end());
          } while (is_duplicate_permutation (permuted_labelling, permutations));
          permutations.push_back (permuted_labelling);
        }
      }


      inline void statistic2pvalue (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& perm_dist,
                                    const vector<value_type>& stats,
                                    vector<value_type>& pvalues)
      {
        vector <value_type> permutations (perm_dist.size(), 0);
        for (ssize_t i = 0; i < perm_dist.size(); i++)
          permutations[i] = perm_dist[i];
        std::sort (permutations.begin(), permutations.end());
        pvalues.resize (stats.size());
        for (size_t i = 0; i < stats.size(); ++i) {
          if (stats[i] > 0.0) {
            value_type pvalue = 1.0;
            for (size_t j = 0; j < permutations.size(); ++j) {
              if (stats[i] < permutations[j]) {
                pvalue = value_type (j) / value_type (permutations.size());
                break;
              }
            }
            pvalues[i] = pvalue;
          }
          else
            pvalues[i] = 0.0;
        }
      }
    }
  }
}

#endif
