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

#ifndef __math_sphere_set_set_h__
#define __math_sphere_set_set_h__

#include <cmath>
#include <sys/types.h>
#include <type_traits>
#include <Eigen/Core>

#include "app.h"
#include "header.h"
#include "types.h"
#include "math/math.h"
#include "math/sphere/sphere.h"




namespace MR {
  namespace Math {
    namespace Sphere {
      namespace Set {



      using index_type = unsigned int;

      using spherical_type = Eigen::Matrix<default_type, Eigen::Dynamic, 2>;
      using cartesian_type = Eigen::Matrix<default_type, Eigen::Dynamic, 3>;
      using matrix_type = Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>;


      extern const char* directions_description;
      MR::App::Option directions_option (const std::string& purpose, const bool ref_description, const std::string& default_set);


      Eigen::MatrixXd load (const std::string& spec, const bool force_singleshell);
      Eigen::MatrixXd load (const Header& H, const bool force_singleshell);



      //! convert matrix of (unit) spherical coordinates to Cartesian coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, cartesian_type>::type
      spherical2cartesian (const MatrixType& az_el)
      {
        assert (az_el.cols() == 2);
        cartesian_type cartesian (az_el.rows(), 3);
        for (ssize_t dir = 0; dir < az_el.rows(); ++dir)
          Sphere::spherical2cartesian (az_el.row (dir), cartesian.row (dir));
        return cartesian;
      }



      //! convert matrix of (unit) Cartesian coordinates to spherical coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, spherical_type>::type
      cartesian2spherical (const MatrixType& cartesian)
      {
        assert (cartesian.cols() == 3);
        spherical_type az_el (cartesian.rows(), 2);
        for (ssize_t dir = 0; dir < cartesian.rows(); ++dir)
          Sphere::cartesian2spherical (cartesian.row (dir), az_el.row (dir));
        return az_el;
      }



      //! ensure that direction matrix is in spherical coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, spherical_type>::type
      to_spherical (const MatrixType& data)
      {
        switch (data.cols()) {
          case 2:
            {
            spherical_type result (data.rows(), 2);
            result << data;
            return result;
            }
          case 3:
            return cartesian2spherical (data);
          default:
            assert (false);
            throw Exception ("Unexpected " + str(data.cols()) + "-column matrix passed to Math::Sphere::Set::to_spherical()");
            return spherical_type();
        }
      }



      //! ensure that direction matrix is in cartesian coordinates
      template <class MatrixType>
      inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, cartesian_type>::type
      to_cartesian (const MatrixType& data)
      {
        switch (data.cols()) {
          case 2:
            return spherical2cartesian (data);
          case 3:
            {
            cartesian_type result (data.rows(), 3);
            result << data;
            return result;
            }
          default:
            assert (false);
            throw Exception ("Unexpected " + str(data.cols()) + "-column matrix passed to Math::Sphere::Set::to_cartesian()");
            return cartesian_type();
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
        if (size_t(M.rows()) != count)
          throw Exception ("Number of entries in direction matrix (" + str(M.rows()) + ") does not match required number (" + str(count) + ")");
        check (M);
      }





      //! Select a subset of the directions within a set
      template <class MatrixType, class IndexVectorType>
      inline MatrixType subset (const MatrixType& data, const IndexVectorType& indices)
      {
        MatrixType result (indices.size(), data.cols());
        for (size_t i = 0; i != indices.size(); i++)
          result.row(i) = data.row(indices[i]);
        return result;
      }






      }
    }
  }
}

#endif

