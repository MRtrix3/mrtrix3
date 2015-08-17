/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 24/02/2012

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

#ifndef __image_registration_symmetric_transform_base_h__
#define __image_registration_symmetric_transform_base_h__

#include "math/vector.h"
#include "math/matrix.h"
// #include "eigen/matrix.h"
// #include "eigen/gsl_eigen.h"
// #include <Eigen/Geometry> 

using namespace MR::Math;

namespace MR
{
  namespace Image
  {
    namespace RegistrationSymmetric
    {
      namespace Transform
      {

        /** \addtogroup Transforms
        @{ */

        /*! A base linear transformation class
         *
         * This class supports the ability to define the centre of rotation.
         * This should be set prior to commencing registration based on the centre of the target image.
         * The translation also should be initialised as moving image centre minus the target image centre.
         *
         */
        template <typename ValueType = float>
        class Base  {
          public:

            typedef ValueType ParameterType;

            Base (size_t number_of_parameters) :
              number_of_parameters(number_of_parameters),
              matrix(3,3),
              matrix_half(3,3),
              matrix_half_inverse(3,3),
              translation(3),
              centre(3),
              offset(3),
              offset_half(3),
              offset_half_inverse(3),
              optimiser_weights (number_of_parameters) {
                matrix.identity();
                matrix_half.identity();
                matrix_half_inverse.identity();
                translation.zero();
                centre.zero();
                offset.zero();
                offset_half.zero();
            }

            template <class OutPointType, class InPointType>
            inline void transform (OutPointType& out, const InPointType& in) const {
              out[0] = matrix(0,0)*in[0] + matrix(0,1)*in[1] + matrix(0,2)*in[2] + offset[0];
              out[1] = matrix(1,0)*in[0] + matrix(1,1)*in[1] + matrix(1,2)*in[2] + offset[1];
              out[2] = matrix(2,0)*in[0] + matrix(2,1)*in[1] + matrix(2,2)*in[2] + offset[2];
            }

            template <class OutPointType, class InPointType>
            inline void transform_half (OutPointType& out, const InPointType& in) const {
              out[0] = matrix_half(0,0)*in[0] + matrix_half(0,1)*in[1] + matrix_half(0,2)*in[2] + offset_half[0];
              out[1] = matrix_half(1,0)*in[0] + matrix_half(1,1)*in[1] + matrix_half(1,2)*in[2] + offset_half[1];
              out[2] = matrix_half(2,0)*in[0] + matrix_half(2,1)*in[1] + matrix_half(2,2)*in[2] + offset_half[2];
            }

            template <class OutPointType, class InPointType>
            inline void transform_half_inverse (OutPointType& out, const InPointType& in) const {
              out[0] = matrix_half_inverse(0,0)*in[0] + matrix_half_inverse(0,1)*in[1] + matrix_half_inverse(0,2)*in[2] + offset_half_inverse[0];
              out[1] = matrix_half_inverse(1,0)*in[0] + matrix_half_inverse(1,1)*in[1] + matrix_half_inverse(1,2)*in[2] + offset_half_inverse[1];
              out[2] = matrix_half_inverse(2,0)*in[0] + matrix_half_inverse(2,1)*in[1] + matrix_half_inverse(2,2)*in[2] + offset_half_inverse[2];
            }

            void set_transform (Matrix<ValueType>& transform) {
              for (size_t row = 0; row < 3; row++) {
                for (size_t col = 0; col < 3; col++)
                  matrix(row, col) = transform(row, col);
                translation[row] = transform(row, 3);
              }
              compute_offset();
              calculate_halfspace_transformations();
            }

            void get_transform (Matrix<ValueType>& transform) const {
              transform.allocate (4,4);
              transform.identity();
              for (size_t row = 0; row < 3; row++) {
                for (size_t col = 0; col < 3; col++)
                  transform(row,col) = matrix(row,col);
                transform(row, 3) = offset[row];
              }
            }

            void get_transform_half (Matrix<ValueType>& transform) const {
              transform.allocate (4,4);
              transform.identity();
              for (size_t row = 0; row < 3; row++) {
                for (size_t col = 0; col < 3; col++)
                  transform(row,col) = matrix_half(row,col);
                transform(row, 3) = offset_half[row];
              }
            }

            void get_transform_half_inverse (Matrix<ValueType>& transform) const {
              transform.allocate (4,4);
              transform.identity();
              for (size_t row = 0; row < 3; row++) {
                for (size_t col = 0; col < 3; col++)
                  transform(row,col) = matrix_half_inverse(row,col);
                transform(row, 3) = offset_half_inverse[row];
              }
            }

            const Matrix<ValueType> get_transform () const {
              Matrix<ValueType> transform(4,4);
              transform.identity();
              for (size_t row = 0; row < 3; row++) {
                for (size_t col = 0; col < 3; col++)
                  transform(row,col) = matrix(row,col);
                transform(row, 3) = offset[row];
              }
              return transform;
            }

            void set_matrix (const Matrix<ValueType>& mat) {
              for (size_t row = 0; row < 3; row++) {
                for (size_t col = 0; col < 3; col++)
                   matrix(row, col) = mat (row, col);
              }
              compute_offset();
              calculate_halfspace_transformations();
            }

            const Matrix<ValueType> get_matrix () const {
              return matrix;
            }

            void set_translation (const Vector<ValueType>& trans) {
              translation = trans;
              compute_offset();
            }

            const Vector<ValueType> get_translation() const {
              return translation;
            }

            void set_centre (const Vector<ValueType>& center) {
              centre = center;
              compute_offset();
            }

            const Vector<ValueType> get_centre() const {
              return centre;
            }

            size_t size() const {
              return number_of_parameters;
            }

            void set_optimiser_weights (Vector<ValueType>& weights) {
              assert(size() == optimiser_weights.size());
                optimiser_weights = weights;
            }

            void get_optimiser_weights (Vector<ValueType>& weights) const {
              weights.allocate (optimiser_weights.size());
              for (size_t i = 0; i < optimiser_weights.size(); i++)
                weights[i] = optimiser_weights[i];
            }

            void get_offset (Math::Vector<ValueType>& offset_out) const {
              offset_out.allocate(3);
              offset_out[0] = offset[0];
              offset_out[1] = offset[1];
              offset_out[2] = offset[2];
            }

            void set_offset (
              const Math::Vector<ValueType>& offset_in,
              const Math::Vector<ValueType>& offset_half_in,
              const Math::Vector<ValueType>& offset_half_inverse_in) {
              offset[0] = offset_in[0];
              offset[1] = offset_in[1];
              offset[2] = offset_in[2];
              offset_half[0] = offset_half_in[0];
              offset_half[1] = offset_half_in[1];
              offset_half[2] = offset_half_in[2];
              offset_half_inverse[0] = offset_half_inverse_in[0];
              offset_half_inverse[1] = offset_half_inverse_in[1];
              offset_half_inverse[2] = offset_half_inverse_in[2];
            }


          protected:

            void compute_offset () {
              for( size_t i = 0; i < 3; i++ ) {
                offset[i] = translation[i] + centre[i];
                offset_half[i] = 0.5 * translation[i] + centre[i];
                offset_half_inverse[i] = - 0.5 * translation[i] + centre[i];
                for( size_t j = 0; j < 3; j++ ){
                  offset[i] -= matrix(i, j) * centre[j];
                  offset_half[i] -= matrix_half(i, j) * centre[j];
                  offset_half_inverse[i] -= matrix_half_inverse(i, j) * centre[j];
                }
              }
            }

            void calculate_halfspace_transformations(){
              Eigen::Matrix<double,3,3> mat;
              Eigen::gslMatrix2eigenMatrix<decltype(matrix),Eigen::Matrix<double,3,3>>(matrix, mat); 
              Eigen::Matrix<double,3,3> mat_sqrt  = mat.sqrt();
              // std::cout << matrix << "\n" << mat << "\n" << mat_sqrt << "\n";
              Eigen::eigenMatrix2gslMatrix<Eigen::Matrix<double,3,3>,decltype(matrix_half)>(mat_sqrt, matrix_half);
              Eigen::eigenMatrix2gslMatrix<Eigen::Matrix<double,3,3>,decltype(matrix_half_inverse)>(mat_sqrt.inverse(), matrix_half_inverse);
              DEBUG("calculate_halfspace_transformations done");
            }

            size_t number_of_parameters;
            Matrix<ValueType> matrix;
            Matrix<ValueType> matrix_half;
            Matrix<ValueType> matrix_half_inverse;
            Vector<ValueType> translation;
            Vector<ValueType> centre;
            Vector<ValueType> offset;
            Vector<ValueType> offset_half;
            Vector<ValueType> offset_half_inverse;
            Vector<ValueType> optimiser_weights;

        };
        //! @}
      }
    }
  }
}

#endif
