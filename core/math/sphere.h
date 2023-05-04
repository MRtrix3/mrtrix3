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

#ifndef __math_sphere_h__
#define __math_sphere_h__

#include <cmath>
#include <sys/types.h>
#include <type_traits>

#include <Eigen/Core>

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



      //! ensure that direction matrix is in spherical coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, Eigen::MatrixXd>::type
      to_spherical (const MatrixType& data)
      {
        switch (data.cols()) {
          case 2: return data;
          case 3: return cartesian2spherical (data);
          default: assert (false); throw Exception ("Unexpected " + str(data.cols()) + "-column matrix passed to Math::Sphere::to_spherical()"); return data;
        }
      }



      //! ensure that direction matrix is in cartesian coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, Eigen::MatrixXd>::type
      to_cartesian (const MatrixType& data)
      {
        switch (data.cols()) {
          case 2: return spherical2cartesian (data);
          case 3: return data;
          default: assert (false); throw Exception ("Unexpected " + str(data.cols()) + "-column matrix passed to Math::Sphere::to_cartesian()"); return data;
        }
      }



      //! check whether a direction matrix provided in spherical coordinates is valid
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, void>::type
      check_spherical (const MatrixType& M)
      {
        if (M.cols() != 2)
          throw Exception ("Direction matrix is not stored in spherical coordinates");
        const Eigen::Array<typename MatrixType::Scalar, 2, 1> min_values = M.colwise().minCoeff();
        const Eigen::Array<typename MatrixType::Scalar, 2, 1> max_values = M.colwise().maxCoeff();
        const typename MatrixType::Scalar az_range = max_values[0] - min_values[0];
        const typename MatrixType::Scalar el_range = max_values[1] - min_values[1];
        if (az_range < Math::pi || el_range < 0.5 * Math::pi || az_range > 2.0 * Math::pi || el_range > Math::pi) {
          WARN ("Values in spherical coordinate direction matrix do not conform to expected range "
                "(azimuth: [" + str(min_values[0]) + " - " + str(max_values[0]) + "]; elevation: [" + str(min_values[1]) + " - " + str(max_values[1]) + "])");
        }
      }



      //! check whether a direction matrix provided in cartesian coordinates is valid
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, void>::type
      check_cartesian (const MatrixType& M)
      {
        if (M.cols() != 3)
          throw Exception ("Direction matrix is not stored in cartesian coordinates");
        auto norms = M.rowwise().norm();
        const typename MatrixType::Scalar min_norm = norms.minCoeff();
        const typename MatrixType::Scalar max_norm = norms.maxCoeff();
        if (min_norm > typename MatrixType::Scalar(1) || max_norm < typename MatrixType::Scalar(1) || max_norm - min_norm > 128 * std::numeric_limits<typename MatrixType::Scalar>::epsilon()) {
          WARN ("Values in cartesian coordinate direction matrix do not conform to expectations "
                "(norms range from " + str(min_norm) + " to " + str(max_norm) + ")");
        }
      }



      //! check whether a direction matrix is valid
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, void>::type
      check (const MatrixType& M)
      {
        switch (M.cols()) {
          case 2: check_spherical (M); break;
          case 3: check_cartesian (M); break;
          default: throw Exception ("Unsupported number of columns (" + str(M.cols()) + ") in direction matrix");
        }
      }



      //! check whether a direction matrix is valid and has the expected number of directions
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, void>::type
      check (const MatrixType& M, const size_t count)
      {
        if (M.rows() != count)
          throw Exception ("Number of entries in direction matrix (" + str(M.rows()) + ") does not match required number (" + str(count) + ")");
        check (M);
      }



      //! normalise a set of Cartesian coordinates
      template <class MatrixType>
      inline void normalise_cartesian (MatrixType& cartesian)
      {
        assert (cartesian.cols() == 3);
        for (ssize_t i = 0; i < cartesian.rows(); i++) {
          auto norm = cartesian.row(i).norm();
          if (norm)
            cartesian.row(i).array() /= norm;
        }
      }



    }
  }
}

#endif
