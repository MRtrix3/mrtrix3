/*
    Copyright 2015 Brain Research Institute, Melbourne, Australia

    Written by Rami Tabbara, 10/07/15.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
        static const BasisMatrix cubic_poly_derivate_operator;
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
            case SplineProcessingType::Derivative:
              _internal_set = &CubicSpline::_set_deriv;
              break;
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
          const auto vec = WeightVector (position*position*position, position*position, position, 1);
          weights = (vec * basis_matrix);
        }

        inline void _set_deriv (T position) {
          const auto vec = WeightVector (position*position*position, position*position, position, 1);
          deriv_weights = (vec * deriv_basis_matrix);
        }

        inline void _set_value_deriv (T position) {
          const auto vec = WeightVector (position*position*position, position*position, position, 1);
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
    CubicSpline<T>::cubic_poly_derivate_operator((BasisMatrix() <<
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
    HermiteSpline<T>::hermite_derivative_basis_mtrx(CubicSpline<T>::cubic_poly_derivate_operator * hermite_basis_mtrx);


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
    UniformBSpline<T>::uniform_bspline_derivative_basis_mtrx(CubicSpline<T>::cubic_poly_derivate_operator * uniform_bspline_basis_mtrx);

  }
}


#endif

