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


#ifndef __math_cubic_spline_h__
#define __math_cubic_spline_h__
namespace MR
{
  namespace Math
  {
    enum SplineProcessingType
    {
      Value = 1,
      Derivative = 2,
      ValueAndDerivative = Value | Derivative
    };


    template <typename T> class CubicSpline
    {
      public:
        typedef Eigen::Matrix<T, 4, 4> BasisMatrix;
        typedef Eigen::Matrix<T, 1, 4> WeightVector;
        WeightVector weights, deriv_weights;

        void set (T position) {
          (this->*(_internal_set)) (position);
        }

        T coef (size_t i) const {
          return (weights[i]);
        }

      protected:
        static const BasisMatrix cubic_poly_derivative_operator;
        const BasisMatrix basis_matrix;
        const BasisMatrix deriv_basis_matrix;

        CubicSpline (SplineProcessingType processType, const BasisMatrix basis_matrix, const BasisMatrix deriv_basis_matrix) :
          basis_matrix (basis_matrix),
          deriv_basis_matrix (deriv_basis_matrix)
        {
          switch (processType) {
            case SplineProcessingType::Value:
              _internal_set = &CubicSpline::_set_value;
              break;
            // Could be used for partial derviative so we need to calculate both deriv and value weights
            case SplineProcessingType::Derivative:
            case SplineProcessingType::ValueAndDerivative:
              _internal_set = &CubicSpline::_set_value_deriv;
              break;
            default:
              break;
            }
        }

      private:

        CubicSpline () {}

        // Function pointer to the internal set function depening on processing type
        void (CubicSpline::*_internal_set) (T);

        inline void _set_value (T position) {
          const T p2 = Math::pow2(position);
          const auto vec = WeightVector (p2 * position, p2, position, 1.0);
          weights = (vec * basis_matrix);
        }

        inline void _set_value_deriv (T position) {
          const T p2 = Math::pow2(position);
          const auto vec = WeightVector (position * p2, p2, position, 1.0);
          weights = (vec * basis_matrix);
          deriv_weights = (vec * deriv_basis_matrix);
        }
    };


    // Hermite spline implementation
    template <typename T> class HermiteSpline :
    public CubicSpline<T> {
      public:
        typedef typename CubicSpline<T>::BasisMatrix BasisMatrix;
        static const BasisMatrix hermite_basis_mtrx;
        static const BasisMatrix hermite_derivative_basis_mtrx;

        HermiteSpline (SplineProcessingType processType)
          : CubicSpline<T> (processType, hermite_basis_mtrx, hermite_derivative_basis_mtrx) {}
    };


    // Uniform bspline implementation
    template <typename T> class UniformBSpline :
    public CubicSpline<T> {
      public:
        typedef typename CubicSpline<T>::BasisMatrix BasisMatrix;
        static const BasisMatrix uniform_bspline_basis_mtrx;
        static const BasisMatrix uniform_bspline_derivative_basis_mtrx;

        UniformBSpline (SplineProcessingType processType)
          : CubicSpline<T> (processType, uniform_bspline_basis_mtrx, uniform_bspline_derivative_basis_mtrx) {}
    };



    // Initialise our static const matrices
    template <typename T>
    const typename CubicSpline<T>::BasisMatrix
    CubicSpline<T>::cubic_poly_derivative_operator((BasisMatrix() <<
      0, 0, 0, 0,
      3, 0, 0, 0,
      0, 2, 0, 0,
      0, 0, 1, 0).finished());


    // Hermite spline
    template <typename T>
    const typename HermiteSpline<T>::BasisMatrix
    HermiteSpline<T>::hermite_basis_mtrx((BasisMatrix() <<
      -0.5,  1.5, -1.5,  0.5,
         1, -2.5,    2, -0.5,
      -0.5,    0,  0.5,    0,
         0,    1,    0,    0).finished());

    template <typename T>
    const typename HermiteSpline<T>::BasisMatrix
    HermiteSpline<T>::hermite_derivative_basis_mtrx(CubicSpline<T>::cubic_poly_derivative_operator * hermite_basis_mtrx);


    // Uniform b-spline
    template <typename T>
    const typename UniformBSpline<T>::BasisMatrix
    UniformBSpline<T>::uniform_bspline_basis_mtrx((1/6.0) * (BasisMatrix() <<
     -1,  3, -3,  1,
      3, -6,  3,  0,
     -3,  0,  3,  0,
      1,  4,  1,  0).finished());

    template <typename T>
    const typename UniformBSpline<T>::BasisMatrix
    UniformBSpline<T>::uniform_bspline_derivative_basis_mtrx(CubicSpline<T>::cubic_poly_derivative_operator * uniform_bspline_basis_mtrx);

  }
}


#endif

