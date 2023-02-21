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

#ifndef __dwi_tensor_h__
#define __dwi_tensor_h__

#include "types.h"

#include "dwi/shells.h"

namespace MR
{
  namespace DWI
  {

    template <typename T, class MatrixType>
      inline Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> grad2bmatrix (const MatrixType& grad, bool dki = false)
    {
      if (grad.cols() > 3) {
        std::unique_ptr<DWI::Shells> shells;
        try {
          shells.reset (new DWI::Shells (grad));
        } catch (...) {
          WARN ("Unable to separate diffusion gradient table into shells; tensor estimation success uncertain");
        }
        if (shells) {
          if (dki) {
            if (shells->count() < 3)
              throw Exception ("Kurtosis tensor estimation requires at least 3 b-value shells");
          } else {
            if (shells->count() < 2)
              throw Exception ("Tensor estimation requires at least 2 b-values");
          }
        }
      }

      Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> bmat (grad.rows(), (dki ? 22 : 7));
      for (ssize_t i = 0; i < grad.rows(); ++i) {
        const T x = grad(i,0);
        const T y = grad(i,1);
        const T z = grad(i,2);
        const T b = (grad.cols() > 3) ? grad(i,3) : T(1.0);

        bmat(i,0)  = T(-1.0);

        bmat(i,1)  = b * x * x * T(1.0);
        bmat(i,2)  = b * y * y * T(1.0);
        bmat(i,3)  = b * z * z * T(1.0);
        bmat(i,4)  = b * x * y * T(2.0);
        bmat(i,5)  = b * x * z * T(2.0);
        bmat(i,6)  = b * y * z * T(2.0);

        if (dki) {
          bmat(i, 7) = -b * b * x * x * x * x * T( 1.0)/T(6.0);
          bmat(i, 8) = -b * b * y * y * y * y * T( 1.0)/T(6.0);
          bmat(i, 9) = -b * b * z * z * z * z * T( 1.0)/T(6.0);
          bmat(i,10) = -b * b * x * x * x * y * T( 4.0)/T(6.0);
          bmat(i,11) = -b * b * x * x * x * z * T( 4.0)/T(6.0);
          bmat(i,12) = -b * b * x * y * y * y * T( 4.0)/T(6.0);
          bmat(i,13) = -b * b * x * z * z * z * T( 4.0)/T(6.0);
          bmat(i,14) = -b * b * y * y * y * z * T( 4.0)/T(6.0);
          bmat(i,15) = -b * b * y * z * z * z * T( 4.0)/T(6.0);
          bmat(i,16) = -b * b * x * x * y * y * T( 6.0)/T(6.0);
          bmat(i,17) = -b * b * x * x * z * z * T( 6.0)/T(6.0);
          bmat(i,18) = -b * b * y * y * z * z * T( 6.0)/T(6.0);
          bmat(i,19) = -b * b * x * x * y * z * T(12.0)/T(6.0);
          bmat(i,20) = -b * b * x * y * y * z * T(12.0)/T(6.0);
          bmat(i,21) = -b * b * x * y * z * z * T(12.0)/T(6.0);
        }
      }
      return bmat;
    }

    template <class MatrixType, class VectorTypeOut, class VectorTypeIn>
      inline void dwi2tensor (VectorTypeOut& dt, const MatrixType& binv, VectorTypeIn& dwi)
    {
      using T = typename VectorTypeIn::Scalar;
      for (ssize_t i = 0; i < dwi.size(); ++i)
        dwi[i] = dwi[i] > T(0.0) ? -std::log (dwi[i]) : T(0.0);
      dt = binv.template middleRows<6>(1) * dwi;
    }


    template <class VectorType> inline typename VectorType::Scalar tensor2ADC (const VectorType& dt)
    {
      using T = typename VectorType::Scalar;
      return (dt[0]+dt[1]+dt[2]) / T (3.0);
    }


    template <class VectorType> inline typename VectorType::Scalar tensor2FA (const VectorType& dt)
    {
      using T = typename VectorType::Scalar;
      T trace = tensor2ADC (dt);
      T a[] = { dt[0]-trace, dt[1]-trace, dt[2]-trace };
      trace = dt[0]*dt[0] + dt[1]*dt[1] + dt[2]*dt[2] + T (2.0) * (dt[3]*dt[3] + dt[4]*dt[4] + dt[5]*dt[5]);
      return trace ?
             std::sqrt (T (1.5) * (a[0]*a[0]+a[1]*a[1]+a[2]*a[2] + T (2.0) * (dt[3]*dt[3]+dt[4]*dt[4]+dt[5]*dt[5])) / trace) :
             T (0.0);
    }


    template <class VectorType> inline typename VectorType::Scalar tensor2RA (const VectorType& dt)
    {
      using T = typename VectorType::Scalar;
      T trace = tensor2ADC (dt);
      T a[] = { dt[0]-trace, dt[1]-trace, dt[2]-trace };
      return trace ?
             sqrt ( (a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+ T (2.0) * (dt[3]*dt[3]+dt[4]*dt[4]+dt[5]*dt[5])) /T (3.0)) / trace :
             T (0.0);
    }

  }
}

#endif
