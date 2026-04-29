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

#ifndef __dwi_shgqi_kernel_h__
#define __dwi_shgqi_kernel_h__

#include <Eigen/Dense>

namespace MR {
  namespace DWI {
    namespace SH_GQI {

      //! Closed-form Funk-Hecke kernel K_l(a) for the GQI sinc projection.
      /*! For even degree l only:
       *      K_l(a) = (4 pi (-1)^{l/2} / a) * integral_0^a j_l(tau) dtau
       *  with limits K_0(0) = 4 pi and K_l(0) = 0 for l >= 2.
       *  Throws if l is odd or negative. Vector input must be non-negative.
       *  Dispatch:
       *    a <= 0.5  -> 30-term power series (machine-precision in this regime)
       *    a >  0.5  -> 64-point Gauss-Legendre quadrature on [0, a].
       */
      Eigen::VectorXd K_l (int l, const Eigen::Ref<const Eigen::VectorXd>& a);

      //! Convenience scalar entry point.
      double K_l (int l, double a);

      //! Stack K_l(a) over even degrees l in {0, 2, ..., L_max}.
      /*! Returns a matrix of shape ((L_max/2 + 1), a.size()) where row r
       *  corresponds to l = 2*r. */
      Eigen::MatrixXd K_matrix (int L_max,
                                const Eigen::Ref<const Eigen::VectorXd>& a);

    }
  }
}

#endif
