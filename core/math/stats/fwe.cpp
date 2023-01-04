/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "math/stats/fwe.h"

#include <algorithm>
#include <types.h>

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      // FIXME Jump based on non-initialised value in the sort
      // Pre-fill the null distribution / stats matrices with NaNs, detect when it's not overwritten
      matrix_type fwe_pvalue (const matrix_type& null_distributions, const matrix_type& statistics)
      {
        assert (null_distributions.cols() == 1 || null_distributions.cols() == statistics.cols());
        matrix_type pvalues (statistics.rows(), statistics.cols());

        auto s2p = [] (const vector<value_type>& null_dist, const matrix_type::ConstColXpr in, matrix_type::ColXpr out)
        {
          for (ssize_t element = 0; element != in.size(); ++element) {
            if (in[element] > 0.0) {
              value_type pvalue = 1.0;
              for (size_t j = 0; j < size_t(null_dist.size()); ++j) {
                if (in[element] < null_dist[j]) {
                  pvalue = value_type(j) / value_type(null_dist.size());
                  break;
                }
              }
              out[element] = pvalue;
            } else {
              out[element] = 0.0;
            }
          }
        };

        if (null_distributions.cols() == 1) { // strong fwe control

          vector<value_type> sorted_null_dist;
          sorted_null_dist.reserve (null_distributions.rows());
          for (ssize_t shuffle = 0; shuffle != null_distributions.rows(); ++shuffle)
            sorted_null_dist.push_back (null_distributions (shuffle, 0));
          std::sort (sorted_null_dist.begin(), sorted_null_dist.end());
          for (ssize_t hypothesis = 0; hypothesis != statistics.cols(); ++hypothesis)
            s2p (sorted_null_dist, statistics.col (hypothesis), pvalues.col (hypothesis));

        } else { // weak fwe control

          for (ssize_t hypothesis = 0; hypothesis != statistics.cols(); ++hypothesis) {
            vector<value_type> sorted_null_dist;
            sorted_null_dist.reserve (null_distributions.rows());
            for (ssize_t shuffle = 0; shuffle != null_distributions.rows(); ++shuffle)
              sorted_null_dist.push_back (null_distributions (shuffle, hypothesis));
            std::sort (sorted_null_dist.begin(), sorted_null_dist.end());
            s2p (sorted_null_dist, statistics.col (hypothesis), pvalues.col (hypothesis));
          }

        }

        return pvalues;
      }




    }
  }
}
