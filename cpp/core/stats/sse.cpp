/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "stats/sse.h"

namespace MR
{
  namespace Stats
  {



    SSE::SSE (const Track::Matrix::Reader& similarity_matrix, const Eigen::Matrix<float, Eigen::Dynamic, 1> weights_sift,
              const value_type dh,
              const value_type E,
              const value_type H,
              const value_type M,
              const bool norm) :
        matrix (similarity_matrix),
        weights (weights_sift),
        dh (dh),
        E (E),
        H (H),
        M (M),
        normalise (norm) { }



        void SSE::operator() (in_column_type stats, out_column_type enhanced_stats) const
        {
          // do I need this to make it thread safe?
          enhanced_stats.setZero();
    
          for (size_t streamline = 0; streamline < matrix.size(); ++streamline) {
            if (stats[streamline] < dh)
            continue;

            // extract both similarity scores and index of similar streamlines
            auto similarities = matrix.get_scores(streamline);
            auto tracks = matrix.get_similar(streamline);

            // norm factor
            float norm_factor = 1.0;
            float sum = 0.0;
            for (ssize_t i = 0; i < similarities.size(); ++i) {
              sum += weights[tracks[i]] * std::pow (similarities[i], M);
            }
            norm_factor = sum;

            // see CFE implementation for reference
            std::vector<float> extents (std::floor (stats[streamline]/dh), float(0));
            for (ssize_t i = 0; i < similarities.size(); ++i) {
              const default_type similarity_stat = stats[tracks[i]];
              if (similarity_stat > dh) {
                const size_t cluster_count = std::min (extents.size(), size_t(std::floor (similarity_stat / dh)));
                for (size_t cluster_index = 0; cluster_index != cluster_count; ++cluster_index) {
                  // sift2 is taken into account here
                  extents[cluster_index] += weights[tracks[i]] * std::pow (similarities[i], M);
                }
              }
            }


            // Pre-calculate h^H
            if (h_pow_H.size() < extents.size()) {
              const size_t old_size = h_pow_H.size();
              h_pow_H.resize (extents.size());
              for (size_t ih = old_size; ih != h_pow_H.size(); ++ih)
                h_pow_H[ih] = std::pow (dh*(ih+1), H);
            }


            for (size_t cluster_index = 0; cluster_index != extents.size(); ++cluster_index) {
              enhanced_stats[streamline] += std::pow (extents[cluster_index], E) * h_pow_H[cluster_index];
            }

            if (normalise)
              enhanced_stats[streamline] /= norm_factor;
          }
    
    
        }



  }
}
