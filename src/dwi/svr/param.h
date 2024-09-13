/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#pragma once

#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>


namespace MR::DWI::SVR
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


