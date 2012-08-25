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

#ifndef __registration_transform_affine_h__
#define __registration_transform_affine_h__

#include "math/vector.h"
#include "math/matrix.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      /** \addtogroup Transforms
      @{ */

      /*! A 3D affine transformation class for registration.
       *
       * This class supports the ability to define the centre of rotation.
       * This should be set prior to commencing registration based on the centre of the target image.
       * The translation also should be initialised as moving image centre minus the target image centre.
       *
       */
      template <typename T = float>
      class Affine  {
        public:

          typedef T ParameterType;

          Affine () :
            matrix_(3,3),
            translation_(3),
            centre_(3),
            offset_(3),
            optimiser_weights_ (12) {
              matrix_.identity();
              translation_.zero();
              centre_.zero();
              offset_.zero();
              for (size_t i = 0; i < 9; i++)
                optimiser_weights_[i] = 0.003;
              for (size_t i = 9; i < 12; i++)
                optimiser_weights_[i] = 1.0;
          }

          template <class PointType>
          inline void transform (PointType& out, const PointType& in) const {
              out[0] = matrix_(0,0)*in[0] + matrix_(0,1)*in[1] + matrix_(0,2)*in[2] + offset_[0];
              out[1] = matrix_(1,0)*in[0] + matrix_(1,1)*in[1] + matrix_(1,2)*in[2] + offset_[1];
              out[2] = matrix_(2,0)*in[0] + matrix_(2,1)*in[1] + matrix_(2,2)*in[2] + offset_[2];
          }

          template <class PointType>
          void get_jacobian_wrt_params (const PointType& p, Matrix<T>& jacobian) const {
            jacobian.resize(3,12);
            jacobian.zero();
            Vector<T> v (3);
            v[0] = p[0] - centre_[0];
            v[1] = p[1] - centre_[1];
            v[2] = p[2] - centre_[2];
            size_t blockOffset = 0;
            for (size_t block = 0; block < 3; block++) {
              for (size_t dim = 0; dim < 3; dim++)
                jacobian (block, blockOffset + dim) = v[dim];
              blockOffset += 3;
            }
            for (size_t dim = 0; dim < 3; dim++) {
              jacobian (dim, blockOffset + dim) = 1.0;
            }
          }

          void set_transform (Matrix<T>& transform) {
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                matrix_(row, col) = transform(row, col);
              translation_[row] = transform(row, 3);
            }
            compute_offset();
          }

          Matrix<T> get_transform () const {
            Matrix<T> transform(4,4);
            transform.identity();
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                transform(row,col) = matrix_(row,col);
              transform(row, 3) = offset_[row];
            }
            return transform;
          }

          void set_parameter_vector (const Math::Vector<T>& param_vector) {
            size_t index = 0;
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                matrix_(row, col) = param_vector[index++];
            }
            for (size_t dim = 0; dim < 3; dim++)
              translation_[dim] = param_vector[index++];
            compute_offset();
          }

          Vector<T> get_parameter_vector () const {
            Vector<T> param_vector(12);
            size_t index = 0;
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                param_vector[index++] = matrix_(row, col);
            }
            for (size_t dim = 0; dim < 3; dim++)
              param_vector[index++] = translation_[dim];
            return param_vector;
          }

          void set_matrix (Matrix<T>& matrix) {
            matrix_ = matrix;
            compute_offset();
          }

          void get_matrix (Matrix<T>& matrix) const {
            matrix.allocate(3, 3);
            for (size_t i = 0; i < 3; i++)
              for (size_t j = 0; j < 3; j++)
                matrix(i, j) = matrix_(i, j);
          }

          void set_translation (Vector<T>& translation) {
            translation_ = translation;
            compute_offset();
          }

          void get_translation (Vector<T>& translation) const {
            translation.allocate(3);
            translation[0] = translation_[0];
            translation[1] = translation_[1];
            translation[2] = translation_[2];
          }

          void set_centre(Vector<T>& centre) {
            centre_ = centre;
            compute_offset();
          }

          void get_centre (Vector<T>& centre) const {
            centre.allocate(3);
            centre[0] = centre_[0];
            centre[1] = centre_[1];
            centre[2] = centre_[2];
          }

          size_t size() const {
            return 12;
          }

          void set_optimiser_weights (Vector<T>& optimiser_weights) {
            assert(size() == optimiser_weights.size());
              optimiser_weights_ = optimiser_weights;
          }

          const Vector<T>& get_optimiser_weights () const {
            return optimiser_weights_;
          }

        protected:

          void compute_offset () {
            for( size_t i = 0; i < 3; i++ ) {
              offset_[i] = translation_[i] + centre_[i];
              for( size_t j = 0; j < 3; j++ )
                offset_[i] -= matrix_(i, j) * centre_[j];
            }
          }


          Matrix<T> matrix_;
          Vector<T> translation_;
          Vector<T> centre_;
          Vector<T> offset_;
          Vector<T> optimiser_weights_; // To weight updates to matrix parameters differently to translation

      };
      //! @}
    }
  }
}

#endif
