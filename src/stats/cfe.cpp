/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "stats/cfe.h"

namespace MR
{
  namespace Stats
  {



    CFE::CFE (const Fixel::Matrix::Reader& connectivity_matrix,
              const value_type dh,
              const value_type E,
              const value_type H,
              const value_type C,
              const bool norm) :
        matrix (connectivity_matrix),
        dh (dh),
        E (E),
        H (H),
        C (C),
        normalise (norm) { }



    void CFE::operator() (in_column_type stats, out_column_type enhanced_stats) const
    {
      enhanced_stats.setZero();
      vector<default_type> connected_stats;
      for (size_t fixel = 0; fixel < matrix.size(); ++fixel) {
        if (stats[fixel] <= 0.0)
          continue;
        auto connections = matrix[fixel];
        // Need to re-normalise based on the value of the power C
        if (C != 1.0) {
          default_type sum = 0.0;
          for (auto& c : connections) {
            c.exponentiate (C);
            sum += c.value();
          }
          connections.normalise (Fixel::Matrix::connectivity_value_type (sum));
        }
        connected_stats.resize (connections.size());
        for (size_t ic = 0; ic != connections.size(); ++ic)
          connected_stats[ic] = stats[connections[ic].index()];
        for (value_type h = this->dh; h < stats[fixel]; h += this->dh) {
          value_type extent = 0.0;
          for (size_t ic = 0; ic != connections.size(); ++ic)
            if (connected_stats[ic] > h)
              extent += connections[ic].value();
          enhanced_stats[fixel] += std::pow (extent, E) * std::pow (h, H);
        }
        if (normalise)
          enhanced_stats[fixel] *= connections.norm_multiplier;
      }
    }



  }
}
