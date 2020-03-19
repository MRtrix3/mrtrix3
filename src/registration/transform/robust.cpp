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
#include "registration/transform/robust.h"
namespace MR
{
  namespace Registration
  {
    void calc_median_trafo (const MR::vector<transform_type>& candid_trafo, const Eigen::Matrix<default_type, 4, 4> & vertices_4,
      const transform_type & trafo_before, transform_type & median_trafo, MR::vector<default_type> & scores) {
      DEBUG("calc_median_trafo");
      const int n_vertices = 4;
      int weiszfeld_iterations = 100;
      default_type weiszfeld_precision = 1e-6;
      const int n_estimates = candid_trafo.size();

      transform_type trafo;
      const Eigen::Matrix<default_type, 3, n_vertices> vertices = vertices_4.template block<3,4>(0,0);
      MR::vector<Eigen::Matrix<default_type, 3, Eigen::Dynamic>> candid_vertices (n_vertices);
      Eigen::Matrix<default_type, 3, Eigen::Dynamic> start_vertices = trafo_before * vertices;

      for (size_t i = 0; i < n_vertices; ++i)
        candid_vertices[i].resize(3,n_estimates);

      for (size_t j =0; j < n_estimates; ++j) {
        for (size_t i = 0; i < n_vertices; ++i) {
          candid_vertices[i].col(j) = candid_trafo[j] * vertices.col(i);
        }
      }

      Eigen::Matrix<default_type, 4, n_vertices> median_vertices_4;
      for (size_t i = 0; i < n_vertices; ++i) {
        Eigen::Matrix<default_type, 3, 1> median_vertex;
        Math::median_weiszfeld (candid_vertices[i], median_vertex, weiszfeld_iterations, weiszfeld_precision);
        median_vertices_4.col(i) << median_vertex, 1.0;
      }

      Eigen::ColPivHouseholderQR<Eigen::Matrix<default_type, 4, n_vertices>> dec(vertices_4.transpose());
      Eigen::Matrix<default_type, 4, 4> median;
      median.transpose() = dec.solve(median_vertices_4.transpose());

      median_trafo.matrix() = median.template block<3,4>(0,0);

      ///////////////////////
      // calculate z score of each candidate
      ///////////////////////
      if (scores.size()) {
        // MAT(candid_vertices[0]);
        // MAT(median_vertices_4.col(0));

        // calculate distance between candidates and median:
        // dist: between 0 and 2. <= 1.
        //   p == reference: 0
        //   p _|_ reference: 1
        //   p = -reference: 2
        //   |p| < |reference|: closer to 1
        for (size_t j = 0; j < n_estimates; ++j)
          scores[j] = 0.0;

        Eigen::MatrixXd P, pn;
        Eigen::Matrix<default_type, 3, n_vertices> R = median_vertices_4.template block<3,n_vertices>(0,0) - start_vertices;
        for (size_t i = 0; i < n_vertices; ++i) {
          default_type rn = R.col(i).norm();
          if (rn < 1.0e-8) // TODO use convergence length
            continue;
          P = candid_vertices[i].colwise() - start_vertices.col(i);
          pn = P.colwise().norm();
          for (size_t j = 0; j < n_estimates; ++j) {
            scores[j] += 1.0 - P.col(j).dot(R.col(i)) / (rn * std::max<default_type>(pn(j), rn));
          }
        }
      }
    }
  }
}
