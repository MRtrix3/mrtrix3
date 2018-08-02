/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __math_sphere_h__
#define __math_sphere_h__

#include <cmath>
#include <sys/types.h>
#include <type_traits>

#include "math/math.h"


namespace MR
{
  namespace Math
  {
    namespace Sphere
    {



      //! convert spherical coordinates to Cartesian coordinates
      template <class VectorType1, class VectorType2>
      inline typename std::enable_if<VectorType1::IsVectorAtCompileTime, void>::type
      spherical2cartesian (const VectorType1& az_el_r, VectorType2&& xyz)
      {
        if (az_el_r.size() == 3) {
          xyz[0] = az_el_r[2] * std::sin (az_el_r[1]) * std::cos (az_el_r[0]);
          xyz[1] = az_el_r[2] * std::sin (az_el_r[1]) * std::sin (az_el_r[0]);
          xyz[2] = az_el_r[2] * cos (az_el_r[1]);
        } else {
          xyz[0] = std::sin (az_el_r[1]) * std::cos (az_el_r[0]);
          xyz[1] = std::sin (az_el_r[1]) * std::sin (az_el_r[0]);
          xyz[2] = cos (az_el_r[1]);
        }
      }



      //! convert matrix of spherical coordinates to Cartesian coordinates
      template <class MatrixType1, class MatrixType2>
      inline typename std::enable_if<!MatrixType1::IsVectorAtCompileTime, void>::type
      spherical2cartesian (const MatrixType1& az_el, MatrixType2&& cartesian)
      {
        cartesian.resize (az_el.rows(), 3);
        for (ssize_t dir = 0; dir < az_el.rows(); ++dir)
          spherical2cartesian (az_el.row (dir), cartesian.row (dir));
      }



      //! convert matrix of spherical coordinates to Cartesian coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, Eigen::MatrixXd>::type
      spherical2cartesian (const MatrixType& az_el)
      {
        Eigen::MatrixXd cartesian (az_el.rows(), 3);
        for (ssize_t dir = 0; dir < az_el.rows(); ++dir)
          spherical2cartesian (az_el.row (dir), cartesian.row (dir));
        return cartesian;
      }



      //! convert Cartesian coordinates to spherical coordinates
      template <class VectorType1, class VectorType2>
      inline typename std::enable_if<VectorType1::IsVectorAtCompileTime, void>::type
      cartesian2spherical (const VectorType1& xyz, VectorType2&& az_el_r)
      {
        auto r = std::sqrt (Math::pow2(xyz[0]) + Math::pow2(xyz[1]) + Math::pow2(xyz[2]));
        az_el_r[0] = std::atan2 (xyz[1], xyz[0]);
        az_el_r[1] = std::acos (xyz[2] / r);
        if (az_el_r.size() == 3)
          az_el_r[2] = r;
      }



      //! convert matrix of Cartesian coordinates to spherical coordinates
      template <class MatrixType1, class MatrixType2>
      inline typename std::enable_if<!MatrixType1::IsVectorAtCompileTime, void>::type
      cartesian2spherical (const MatrixType1& cartesian, MatrixType2&& az_el, bool include_r = false)
      {
        az_el.allocate (cartesian.rows(), include_r ? 3 : 2);
        for (ssize_t dir = 0; dir < cartesian.rows(); ++dir)
          cartesian2spherical (cartesian.row (dir), az_el.row (dir));
      }



      //! convert matrix of Cartesian coordinates to spherical coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, Eigen::MatrixXd>::type
      cartesian2spherical (const MatrixType& cartesian, bool include_r = false)
      {
        Eigen::MatrixXd az_el (cartesian.rows(), include_r ? 3 : 2);
        for (ssize_t dir = 0; dir < cartesian.rows(); ++dir)
          cartesian2spherical (cartesian.row (dir), az_el.row (dir));
        return az_el;
      }



    }
  }
}

#endif
