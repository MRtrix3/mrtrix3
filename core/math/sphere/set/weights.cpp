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

#include "math/sphere/set/weights.h"

#include "math/sphere/SH.h"
#include "math/sphere/set/set.h"



namespace MR {
  namespace Math {
    namespace Sphere {
      namespace Set {



      Weights::Weights (const Eigen::MatrixXd& dirs) :
          data (dirs.size())
      {
        const size_t calibration_lmax = SH::LforN (dirs.rows()) + 2;
        auto calibration_SH2A = SH::init_transform (to_spherical (dirs), calibration_lmax);
        const size_t num_basis_fns = calibration_SH2A.cols();

        // Integrating an FOD with constant amplitude 1 (l=0 term = sqrt(4pi)) should produce a value of 4pi
        // Every other integral should produce zero
        Eigen::Matrix<default_type, Eigen::Dynamic, 1> integral_results = Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero (num_basis_fns);
        integral_results[0] = 2.0 * sqrt(Math::pi);

        // Problem matrix: One row for each SH basis function, one column for each samping direction
        Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> A;
        A.resize (num_basis_fns, dirs.rows());

        for (size_t basis_fn_index = 0; basis_fn_index != num_basis_fns; ++basis_fn_index) {
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> SH_in = Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero (num_basis_fns);
          SH_in[basis_fn_index] = 1.0;
          A.row (basis_fn_index) = calibration_SH2A * SH_in;
        }

        data = A.householderQr().solve (integral_results);
      }



      }
    }
  }
}

