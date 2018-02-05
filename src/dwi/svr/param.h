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


#ifndef __dwi_svr_param_h__
#define __dwi_svr_param_h__

#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>


namespace MR
{
  namespace DWI
  {
    namespace SVR
    {

      /* Exponential Lie mapping on SE(3). */
      template <typename VectorType>
      Eigen::Matrix4f se3exp(const VectorType& v)
      {
        Eigen::Matrix4f A, T; A.setZero();
        A(0,3) = v[0]; A(1,3) = v[1]; A(2,3) = v[2];
        A(2,1) = v[3]; A(1,2) = -v[3];
        A(0,2) = v[4]; A(2,0) = -v[4];
        A(1,0) = v[5]; A(0,1) = -v[5];
        T = A.exp();
        return T;
      }


      /* Logarithmic Lie mapping on SE(3). */
      Eigen::Matrix<float, 6, 1> se3log(const Eigen::Matrix4f& T)
      {
        Eigen::Matrix4f A = T.log();
        Eigen::Matrix<float, 6, 1> v;
        v[0] = A(0,3); v[1] = A(1,3); v[2] = A(2,3);
        v[3] = (A(2,1) - A(1,2)) / 2;
        v[4] = (A(0,2) - A(2,0)) / 2;
        v[5] = (A(1,0) - A(0,1)) / 2;
        return v;
      }


    }
  }
}

#endif


