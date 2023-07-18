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
        if (stats[fixel] < dh)
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
        // Rather than allocating data for the stats and then looping over dh,
        //   divide statistic by dh to determine the number of cluster sizes that should
        //   be incremented, and dynamically increment all cluster sizes for that
        //   particular connected fixel
        vector<Fixel::Matrix::connectivity_value_type> extents (std::floor (stats[fixel]/dh), Fixel::Matrix::connectivity_value_type(0));
        for (const auto& connection : connections) {
          const default_type connection_stat = stats[connection.index()];
          if (connection_stat > dh) {
            const size_t cluster_count = std::min (extents.size(), size_t(std::floor (connection_stat / dh)));
            for (size_t cluster_index = 0; cluster_index != cluster_count; ++cluster_index)
              extents[cluster_index] += connection.value();
          }
        }
        // Pre-calculate h^H
        if (h_pow_H.size() < extents.size()) {
          const size_t old_size = h_pow_H.size();
          h_pow_H.resize (extents.size());
          for (size_t ih = old_size; ih != h_pow_H.size(); ++ih)
            h_pow_H[ih] = std::pow (dh*(ih+1), H);
        }
        for (size_t cluster_index = 0; cluster_index != extents.size(); ++cluster_index)
          enhanced_stats[fixel] += std::pow (extents[cluster_index], E) * h_pow_H[cluster_index];
        if (normalise)
          enhanced_stats[fixel] *= connections.norm_multiplier;
      }
    }



  }
}
