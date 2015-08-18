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

#ifndef __registration_transform_base_h__
#define __registration_transform_base_h__

#include "types.h"

namespace MR
{
  namespace Registration
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
      class Base  {
        public:

          typedef default_type ParameterType;

          Base (size_t number_of_parameters) :
            number_of_parameters(number_of_parameters),
            optimiser_weights (number_of_parameters) {
              matrix.setIdentity();
              translation.setZero();
              centre.setZero();
              offset.setZero();
          }

          template <class OutPointType, class InPointType>
          inline void transform (OutPointType& out, const InPointType& in) const {
            out[0] = matrix(0,0)*in[0] + matrix(0,1)*in[1] + matrix(0,2)*in[2] + offset[0];
            out[1] = matrix(1,0)*in[0] + matrix(1,1)*in[1] + matrix(1,2)*in[2] + offset[1];
            out[2] = matrix(2,0)*in[0] + matrix(2,1)*in[1] + matrix(2,2)*in[2] + offset[2];
          }

          void set_transform (transform_type& transform) {
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                matrix(row, col) = transform(row, col);
              translation[row] = transform(row, 3);
            }
            compute_offset();
          }

          transform_type get_transform () const {
            transform_type transform;
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                transform(row,col) = matrix(row,col);
              transform(row, 3) = offset[row];
            }
            return transform;
          }


          void set_matrix (const Eigen::Matrix<default_type, 3, 3>& mat) {
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                 matrix(row, col) = mat (row, col);
            }
            compute_offset();
          }

          const Eigen::Vector3 get_matrix () const {
            return matrix;
          }

          void set_translation (const Eigen::Vector3& trans) {
            translation = trans;
            compute_offset();
          }

          const Eigen::Vector3 get_translation() const {
            return translation;
          }

          void set_centre (const Eigen::Vector3& centre_in) {
            centre = centre_in;
            compute_offset();
          }

          const Eigen::Vector3 get_centre() const {
            return centre;
          }

          size_t size() const {
            return number_of_parameters;
          }

          void set_optimiser_weights (Eigen::Vector3& weights) {
            assert(size() == optimiser_weights.size());
              optimiser_weights = weights;
          }

          Eigen::VectorXd get_optimiser_weights () const {
            return optimiser_weights;
          }

          Eigen::Vector3 get_offset () const {
            return offset;
          }

          void set_offset (const Eigen::Vector3& offset_in) {
            offset = offset_in;
          }


        protected:

          void compute_offset () {
            for( size_t i = 0; i < 3; i++ ) {
              offset[i] = translation[i] + centre[i];
              for( size_t j = 0; j < 3; j++ )
                offset[i] -= matrix(i, j) * centre[j];
            }
          }

          size_t number_of_parameters;
          Eigen::Matrix<default_type, 3, 3> matrix;
          Eigen::Vector3 translation;
          Eigen::Vector3 centre;
          Eigen::Vector3 offset;
          Eigen::VectorXd optimiser_weights;

      };
      //! @}
    }
  }
}

#endif
