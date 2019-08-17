/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include "stats/cfe.h"

namespace MR
{
  namespace Stats
  {



    CFE::CFE (const Fixel::Matrix::norm_matrix_type& connectivity_matrix,
              const value_type dh,
              const value_type E,
              const value_type H) :
        connectivity_matrix (connectivity_matrix),
        dh (dh),
        E (E),
        H (H) { }



    void CFE::operator() (in_column_type stats, out_column_type enhanced_stats) const
    {
      enhanced_stats.setZero();
      vector<Fixel::Matrix::NormElement>::const_iterator connected_fixel;
      for (size_t fixel = 0; fixel < connectivity_matrix.size(); ++fixel) {
        for (value_type h = this->dh; h < stats[fixel]; h +=  this->dh) {
          value_type extent = 0.0;
          for (connected_fixel = connectivity_matrix[fixel].begin(); connected_fixel != connectivity_matrix[fixel].end(); ++connected_fixel)
            if (stats[connected_fixel->index()] > h)
              extent += connected_fixel->value();
          enhanced_stats[fixel] += std::pow (extent, E) * std::pow (h, H);
        }
        enhanced_stats[fixel] *= connectivity_matrix[fixel].norm_multiplier;
      }
    }



  }
}
