/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
        matrix_type pvalues (statistics.rows(), statistics.cols());
        for (ssize_t contrast = 0; contrast != statistics.cols(); ++contrast) {
          vector<value_type> sorted_null_dist;
          sorted_null_dist.reserve (null_distributions.rows());
          for (ssize_t perm = 0; perm != null_distributions.rows(); ++perm)
            sorted_null_dist.push_back (null_distributions (perm, contrast));
          std::sort (sorted_null_dist.begin(), sorted_null_dist.end());
          for (ssize_t element = 0; element != statistics.rows(); ++element) {
            if (statistics (element, contrast) > 0.0) {
              value_type pvalue = 1.0;
              for (size_t j = 0; j < size_t(sorted_null_dist.size()); ++j) {
                if (statistics(element, contrast) < sorted_null_dist[j]) {
                  pvalue = value_type(j) / value_type(sorted_null_dist.size());
                  break;
                }
              }
              pvalues(element, contrast) = pvalue;
            } else {
              pvalues(element, contrast) = 0.0;
            }
          }
        }
        return pvalues;
      }




    }
  }
}
