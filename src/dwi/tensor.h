/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tensor_h__
#define __dwi_tensor_h__

#include "types.h"

namespace MR
{
  namespace DWI
  {

    template <typename T, class MatrixType> 
      inline Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> grad2bmatrix (const MatrixType& grad, bool dki = false)
    {
      Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> bmat (grad.rows(), (dki ? 22 : 7));
      for (ssize_t i = 0; i < grad.rows(); ++i) {
        bmat (i,0)  = grad(i,3) *  grad(i,0) * grad(i,0);
        bmat (i,1)  = grad(i,3) *  grad(i,1) * grad(i,1);
        bmat (i,2)  = grad(i,3) *  grad(i,2) * grad(i,2);
        bmat (i,3)  = grad(i,3) *  grad(i,0) * grad(i,1) * 2;
        bmat (i,4)  = grad(i,3) *  grad(i,0) * grad(i,2) * 2;
        bmat (i,5)  = grad(i,3) *  grad(i,1) * grad(i,2) * 2;
        bmat (i,6)  = -1.0;
        if (dki) {
          bmat (i,7)  = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,0) * grad(i,0) * grad(i,0) * 1/6;
          bmat (i,8)  = -grad(i,3) * grad(i,3) * grad(i,1) * grad(i,1) * grad(i,1) * grad(i,1) * 1/6;
          bmat (i,9)  = -grad(i,3) * grad(i,3) * grad(i,2) * grad(i,2) * grad(i,2) * grad(i,2) * 1/6;
          bmat (i,10) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,0) * grad(i,0) * grad(i,1) * 2/3;
          bmat (i,11) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,0) * grad(i,0) * grad(i,2) * 2/3;
          bmat (i,12) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,1) * grad(i,1) * grad(i,1) * 2/3;
          bmat (i,13) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,2) * grad(i,2) * grad(i,2) * 2/3;
          bmat (i,14) = -grad(i,3) * grad(i,3) * grad(i,1) * grad(i,1) * grad(i,1) * grad(i,2) * 2/3;
          bmat (i,15) = -grad(i,3) * grad(i,3) * grad(i,1) * grad(i,2) * grad(i,2) * grad(i,2) * 2/3;
          bmat (i,16) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,0) * grad(i,1) * grad(i,1);
          bmat (i,17) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,0) * grad(i,2) * grad(i,2);
          bmat (i,18) = -grad(i,3) * grad(i,3) * grad(i,1) * grad(i,1) * grad(i,2) * grad(i,2);
          bmat (i,19) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,0) * grad(i,1) * grad(i,2) * 2;
          bmat (i,20) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,1) * grad(i,1) * grad(i,2) * 2;
          bmat (i,21) = -grad(i,3) * grad(i,3) * grad(i,0) * grad(i,1) * grad(i,2) * grad(i,2) * 2;
        }
      }
      return bmat;
    }

    template <class MatrixType, class VectorTypeOut, class VectorTypeIn>
      inline void dwi2tensor (VectorTypeOut& dt, const MatrixType& binv, VectorTypeIn& dwi)
    {
      typedef typename VectorTypeIn::Scalar T;
      for (ssize_t i = 0; i < dwi.size(); ++i)
        dwi[i] = dwi[i] > T(0.0) ? -std::log (dwi[i]) : T(0.0);
      dt = binv * dwi;
    }


    template <class VectorType> inline typename VectorType::Scalar tensor2ADC (const VectorType& dt)
    {
      typedef typename VectorType::Scalar T;
      return (dt[0]+dt[1]+dt[2]) / T (3.0);
    }


    template <class VectorType> inline typename VectorType::Scalar tensor2FA (const VectorType& dt)
    {
      typedef typename VectorType::Scalar T;
      T trace = tensor2ADC (dt);
      T a[] = { dt[0]-trace, dt[1]-trace, dt[2]-trace };
      trace = dt[0]*dt[0] + dt[1]*dt[1] + dt[2]*dt[2] + T (2.0) * (dt[3]*dt[3] + dt[4]*dt[4] + dt[5]*dt[5]);
      return trace ?
             std::sqrt (T (1.5) * (a[0]*a[0]+a[1]*a[1]+a[2]*a[2] + T (2.0) * (dt[3]*dt[3]+dt[4]*dt[4]+dt[5]*dt[5])) / trace) :
             T (0.0);
    }


    template <class VectorType> inline typename VectorType::Scalar tensor2RA (const VectorType& dt)
    {
      typedef typename VectorType::Scalar T;
      T trace = tensor2ADC (dt);
      T a[] = { dt[0]-trace, dt[1]-trace, dt[2]-trace };
      return trace ?
             sqrt ( (a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+ T (2.0) * (dt[3]*dt[3]+dt[4]*dt[4]+dt[5]*dt[5])) /T (3.0)) / trace :
             T (0.0);
    }

  }
}

#endif
