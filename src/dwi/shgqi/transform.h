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

#ifndef __dwi_shgqi_transform_h__
#define __dwi_shgqi_transform_h__

#include <Eigen/Dense>

namespace MR {
  namespace DWI {
    namespace SH_GQI {

      struct Params {
        int    L_max = 8;       //!< maximum even SH degree
        double sigma = 1.25;    //!< dimensionless GQI sampling-length factor
        double D     = 3.0e-3;  //!< white-matter diffusivity (mm^2/s)
        double mu    = 6.0e-3;  //!< Laplace-Beltrami strength
        double p     = 1.0;     //!< Laplace-Beltrami exponent
      };

      //! Per-gradient kernel argument a_i = sigma * sqrt(6 D b_i) (Yeh 2010 Eq 9).
      Eigen::VectorXd kernel_arguments (
          const Eigen::Ref<const Eigen::VectorXd>& bvals,
          double sigma, double D);

      //! Build the (k x G) closed-form transfer matrix M with k = NforL(L_max),
      //  such that c = M * E for any per-voxel signal vector E.
      /*! \a grad is the MRtrix3 gradient table of shape (G x 4) = [gx, gy, gz, b].
       *  Rows with b == 0 contribute only to the c_00 coefficient through
       *  K_0(0) = 4 pi; their gradient direction is irrelevant.
       *  Rows with b > 0 must have non-zero gradient direction (an Exception
       *  is thrown otherwise). The returned SH coefficients are in the
       *  MRtrix3 basis convention (Math::SH order). */
      Eigen::MatrixXd transfer_matrix (
          const Eigen::Ref<const Eigen::MatrixXd>& grad,
          const Params& params);

    }
  }
}

#endif
