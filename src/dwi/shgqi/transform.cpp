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

#include "dwi/shgqi/transform.h"
#include "dwi/shgqi/kernel.h"

#include <cmath>

#include "exception.h"
#include "mrtrix.h"
#include "math/SH.h"

namespace MR {
  namespace DWI {
    namespace SH_GQI {

      Eigen::VectorXd kernel_arguments (
          const Eigen::Ref<const Eigen::VectorXd>& bvals,
          const double sigma, const double D)
      {
        Eigen::VectorXd a (bvals.size());
        const double scale = sigma * std::sqrt (6.0 * D);
        for (Eigen::Index i = 0; i < bvals.size(); ++i)
          a[i] = scale * std::sqrt (std::max (bvals[i], 0.0));
        return a;
      }

      Eigen::MatrixXd transfer_matrix (
          const Eigen::Ref<const Eigen::MatrixXd>& grad,
          const Params& params)
      {
        if (grad.cols() != 4)
          throw Exception ("SH_GQI::transfer_matrix expects gradient table with 4 columns [gx gy gz b]; got " + str(grad.cols()));
        if (params.L_max < 0 || (params.L_max & 1))
          throw Exception ("SH_GQI: L_max must be a non-negative even integer (got " + str(params.L_max) + ")");
        if (params.sigma <= 0.0 || params.D <= 0.0)
          throw Exception ("SH_GQI: sigma and D must be positive");
        if (params.mu < 0.0)
          throw Exception ("SH_GQI: mu must be non-negative");

        const Eigen::Index G = grad.rows();
        const int          k = Math::SH::NforL (params.L_max);

        // Extract b-values and unit gradient directions, validating the geometry.
        Eigen::VectorXd bvals = grad.col (3);
        Eigen::MatrixXd dirs (G, 3);
        constexpr double DIR_EPS = 1e-6;
        for (Eigen::Index i = 0; i < G; ++i) {
          const double b = bvals[i];
          const double n = grad.row (i).head<3>().norm();
          if (b > 0.0 && n < DIR_EPS)
            throw Exception ("SH_GQI: gradient row " + str(i) + " has b > 0 but zero direction");
          if (n < DIR_EPS) {
            dirs.row (i) << 1.0, 0.0, 0.0;          // unused: K_l(0)=0 for l>0
          } else {
            dirs.row (i) = grad.row (i).head<3>() / n;
          }
        }

        // Per-shell kernel arguments and per-degree kernels.
        Eigen::VectorXd a_vals = kernel_arguments (bvals, params.sigma, params.D);
        Eigen::MatrixXd K       = K_matrix (params.L_max, a_vals);    // (L/2+1) x G

        // SH design matrix in MRtrix3 (Descoteaux-style) basis, even orders only.
        Eigen::MatrixXd Y = Math::SH::init_transform_cart (dirs, params.L_max); // G x k

        // Laplace-Beltrami diagonal shrinkage in the j(l,m) ordering.
        Eigen::VectorXd shrink (k);
        for (int l = 0; l <= params.L_max; l += 2) {
          const double eig    = double (l) * double (l + 1);
          const double factor = 1.0 / (1.0 + params.mu * std::pow (eig, params.p));
          for (int m = -l; m <= l; ++m)
            shrink[Math::SH::index (l, m)] = factor;
        }

        // Assemble M(j, i) = shrink[j] * K_{l(j)}(a_i) * Y_lm(qhat_i)
        Eigen::MatrixXd M (k, G);
        for (int l = 0; l <= params.L_max; l += 2) {
          const Eigen::RowVectorXd Kl_row = K.row (l / 2);
          for (int m = -l; m <= l; ++m) {
            const size_t j = Math::SH::index (l, m);
            // Y.col(j) is (G x 1); Kl_row is (1 x G)
            M.row (j) = shrink[j] * Y.col (j).transpose().array() * Kl_row.array();
          }
        }
        return M;
      }

    }
  }
}
