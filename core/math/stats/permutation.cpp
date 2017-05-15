/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "math/stats/permutation.h"
#include "math/math.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {
      namespace Permutation
      {



        bool is_duplicate (const vector<size_t>& v1, const vector<size_t>& v2)
        {
          for (size_t i = 0; i < v1.size(); i++) {
            if (v1[i] != v2[i])
              return false;
          }
          return true;
        }



        bool is_duplicate (const vector<size_t>& perm,
                           const vector<vector<size_t> >& previous_permutations)
        {
          for (size_t p = 0; p < previous_permutations.size(); p++) {
            if (is_duplicate (perm, previous_permutations[p]))
              return true;
          }
          return false;
        }



        void generate (const size_t num_perms,
                       const size_t num_subjects,
                       vector<vector<size_t> >& permutations,
                       const bool include_default)
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
            } while (is_duplicate (permuted_labelling, permutations));
            permutations.push_back (permuted_labelling);
          }
        }



        void statistic2pvalue (const vector_type& perm_dist, const vector_type& stats, vector_type& pvalues)
        {
          vector<value_type> permutations;
          permutations.reserve (perm_dist.size());
          for (ssize_t i = 0; i != perm_dist.size(); ++i)
            permutations.push_back (perm_dist[i]);
          std::sort (permutations.begin(), permutations.end());
          pvalues.resize (stats.size());
          for (size_t i = 0; i < size_t(stats.size()); ++i) {
            if (stats[i] > 0.0) {
              value_type pvalue = 1.0;
              for (size_t j = 0; j < size_t(permutations.size()); ++j) {
                if (stats[i] < permutations[j]) {
                  pvalue = value_type(j) / value_type(permutations.size());
                  break;
                }
              }
              pvalues[i] = pvalue;
            } else {
              pvalues[i] = 0.0;
            }
          }
        }



        vector<vector<size_t> > load_permutations_file (const std::string& filename) {
          vector<vector<size_t> > temp = load_matrix_2D_vector<size_t> (filename);
          if (!temp.size())
            throw Exception ("no data found in permutations file: " + str(filename));

          size_t min_value = *std::min_element (std::begin (temp[0]), std::end (temp[0]));
          if (min_value > 1)
            throw Exception ("indices for relabelling in permutations file must start from either 0 or 1");

          vector<vector<size_t> > permutations (temp[0].size(), vector<size_t>(temp.size()));
          for (vector<size_t>::size_type i = 0; i < temp[0].size(); i++) {
            for (vector<size_t>::size_type j = 0; j < temp.size(); j++) {
              if (!temp[j][i])
                throw Exception ("Pre-defined permutation labelling file \"" + filename + "\" contains zeros; labels should be indexed from one");
              permutations[i][j] = temp[j][i] - min_value;
            }
          }
          return permutations;
        }



      }
    }
  }
}
