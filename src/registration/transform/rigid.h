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

#ifndef __registration_transform_rigid_h__
#define __registration_transform_rigid_h__

#include "math/vector.h"
#include "math/matrix.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

    class VersorUpdate {
      public:
        template <typename ValueType>
          inline bool operator() (Math::Vector<ValueType>& newx,
                                   const Math::Vector<ValueType>& x,
                                   const Math::Vector<ValueType>& g,
                                   ValueType step_size) {

            Vector<ValueType> right_part (3);
            right_part[0] = x[0];
            right_part[1] = x[1];
            right_part[2] = x[2];

            ValueType sinangle2 =  Math::norm (right_part);
            if ( sinangle2 > 1.0)
              throw Exception ("versor axis has a magnitude greater than 1");

            ValueType cosangle2 =  Math::sqrt (1.0 - sinangle2 * sinangle2);

            Vector<ValueType> current_rot (4);
            current_rot[0] = right_part[0];
            current_rot[1] = right_part[1];
            current_rot[2] = right_part[2];
            current_rot[4] = cosangle2;

             //The gradient indicate the contribution of each one
            // of the axis to the direction of highest change in
            // the function
            Vector<ValueType> axis (3);
            axis[0] = -g[0];
            axis[1] = -g[1];
            axis[2] = -g[2];

            // gradientRotation is a rotation along the
            // versor direction which maximize the
            // variation of the cost function in question.
            // An additional Exponentiation produce a jump
            // of a particular length along the versor gradient
            // direction.

            Vector<ValueType> g_rot (4);
            const ValueType vectorNorm = Math::norm (axis);
            const ValueType angle = step_size * vectorNorm;
            cosangle2 = Math::cos(angle / 2.0);
            sinangle2 = Math::sin(angle / 2.0);
            const ValueType factor = sinangle2 / vectorNorm;
            g_rot[0] = axis[0] * factor;
            g_rot[1] = axis[1] * factor;
            g_rot[2] = axis[2] * factor;
            g_rot[3] = cosangle2;

            // Composing the currentRotation with the gradientRotation
            // produces the new Rotation versor
            Vector<ValueType> new_rotation(4);
            newx[0] =  current_rot[3] * g_rot[0] - current_rot[2] * g_rot[1] + current_rot[1] * g_rot[2] + current_rot[0] * g_rot[3];
            newx[1] =  current_rot[2] * g_rot[0] + current_rot[3] * g_rot[1] - current_rot[0] * g_rot[2] + current_rot[1] * g_rot[3];
            newx[2] = -current_rot[1] * g_rot[0] + current_rot[0] * g_rot[1] + current_rot[3] * g_rot[2] + current_rot[2] * g_rot[3];
//            new_rotation[3] = -current_rot[0] * g_rot[0] - current_rot[1] * g_rot[1] - current_rot[2] * g_rot[2] + current_rot[3] * g_rot[3];
            newx[3] = x[3] - step_size * g[3];
            newx[4] = x[4] - step_size * g[4];
            newx[5] = x[5] - step_size * g[5];

            bool changed = false;
            for (size_t n = 0; n < x.size(); ++n) {
              if (newx[n] != x[n])
                changed = true;
            }
            return changed;
        }
    };

      /** \addtogroup Transforms
      @{ */

      /*! A 3D rigid transformation class for registration.
       *
       * This class defines a rigid transform using 6 parameters. The first 3 parameters define rotation using a versor (unit quaternion),
       * while the last 3 parameters define translation. Note that since the versor parameters do not define a vector space, any updates
       * must be performed using a versor composition (not an addition). This can be achieved by passing the update_parameters method
       * from this class as a function pointer to the gradient_decent run method.
       *
       * This class supports the ability to define the centre of rotation. This should be set prior to commencing registration based on
       * the centre of the target image. The translation also should be initialised as moving image centre minus the target image centre.
       * This can done automatically using functions available in  src/registration/transform/initialiser.h
       *
       */
      template <typename ValueType = float>
      class Rigid  {
        public:

          typedef ValueType ParameterType;

          Rigid () :
            versor_(4),
            matrix_(3,3),
            translation_(3),
            centre_(3),
            offset_(3),
            optimiser_weights_ (6) {
              matrix_.identity();
              translation_.zero();
              centre_.zero();
              offset_.zero();
              for (size_t i = 0; i < 3; i++)
                optimiser_weights_[i] = 0.003;
              for (size_t i = 3; i < 6; i++)
                optimiser_weights_[i] = 1.0;
          }

          template <class PointType>
          inline void transform (const PointType& in, PointType& out) const {
              out[0] = matrix_(0,0)*in[0] + matrix_(0,1)*in[1] + matrix_(0,2)*in[2] + offset_[0];
              out[1] = matrix_(1,0)*in[0] + matrix_(1,1)*in[1] + matrix_(1,2)*in[2] + offset_[1];
              out[2] = matrix_(2,0)*in[0] + matrix_(2,1)*in[1] + matrix_(2,2)*in[2] + offset_[2];
          }

          template <class PointType>
          void get_jacobian_wrt_params (const PointType& p, Matrix<ValueType>& jacobian) const {

            const ValueType vx = versor_[0];
            const ValueType vy = versor_[1];
            const ValueType vz = versor_[2];
            const ValueType vw = versor_[3];

            jacobian.resize(3, 6);
            jacobian.zero();

            const double px = p[0] - centre_[0];
            const double py = p[1] - centre_[1];
            const double pz = p[2] - centre_[2];

            const double vxx = vx * vx;
            const double vyy = vy * vy;
            const double vzz = vz * vz;
            const double vww = vw * vw;

            const double vxy = vx * vy;
            const double vxz = vx * vz;
            const double vxw = vx * vw;

            const double vyz = vy * vz;
            const double vyw = vy * vw;
            const double vzw = vz * vw;

            jacobian(0,0) = 2.0 * ( ( vyw + vxz ) * py + ( vzw - vxy ) * pz ) / vw;
            jacobian(1,0) = 2.0 * ( ( vyw - vxz ) * px   - 2 * vxw   * py + ( vxx - vww ) * pz ) / vw;
            jacobian(2,0) = 2.0 * ( ( vzw + vxy ) * px + ( vww - vxx ) * py   - 2 * vxw   * pz ) / vw;

            jacobian(0,1) = 2.0 * ( -2 * vyw  * px + ( vxw + vyz ) * py + ( vww - vyy ) * pz ) / vw;
            jacobian(1,1) = 2.0 * ( ( vxw - vyz ) * px + ( vzw + vxy ) * pz ) / vw;
            jacobian(2,1) = 2.0 * ( ( vyy - vww ) * px + ( vzw - vxy ) * py   - 2 * vyw   * pz ) / vw;

            jacobian(0,2) = 2.0 * ( -2 * vzw  * px + ( vzz - vww ) * py + ( vxw - vyz ) * pz ) / vw;
            jacobian(1,2) = 2.0 * ( ( vww - vzz ) * px   - 2 * vzw   * py + ( vyw + vxz ) * pz ) / vw;
            jacobian(2,2) = 2.0 * ( ( vxw + vyz ) * px + ( vyw - vxz ) * py ) / vw;

            jacobian(0,3) = 1.0;
            jacobian(1,4) = 1.0;
            jacobian(2,5) = 1.0;
          }

//          void set_transform (Matrix<T>& transform) {
//            for (size_t row = 0; row < 3; row++) {
//              for (size_t col = 0; col < 3; col++)
//                matrix_(row, col) = transform(row, col);
//              translation_[row] = transform(row, 3);
//            }
//            compute_offset();
//          }

          void get_transform (Matrix<ValueType>& transform) const {
            transform.allocate (4,4);
            transform.identity();
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                transform(row,col) = matrix_(row,col);
              transform(row, 3) = offset_[row];
            }
          }

          void set_parameter_vector (const Math::Vector<ValueType>& param_vector) {
            Vector<ValueType> axis(3);
            double norm = param_vector[0] * param_vector[0];
            axis[0] = param_vector[0];
            norm += param_vector[1] * param_vector[1];
            axis[1] = param_vector[1];
            norm += param_vector[2] * param_vector[2];
            axis[2] = param_vector[2];
            if( norm > 0 )
              norm = Math::sqrt(norm);

            double epsilon = 1e-10;
            if( norm >= 1.0 - epsilon )
              axis /= ( norm + epsilon * norm );

            const ValueType sinangle2 =  Math::norm (axis);
            if ( sinangle2 > 1.0)
              throw Exception ("versor axis has a magnitude greater than 1");

            const ValueType cosangle2 =  Math::sqrt (1.0 - sinangle2 * sinangle2);

            versor_[0] = axis[0];
            versor_[1] = axis[1];
            versor_[2] = axis[2];
            versor_[4] = cosangle2;
            compute_matrix();

            translation_[0] = param_vector[3];
            translation_[1] = param_vector[4];
            translation_[2] = param_vector[5];
            compute_offset();
          }

          void get_parameter_vector (Vector<ValueType>& param_vector) const {
            param_vector.allocate (6);
            size_t index = 0;
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                param_vector[index++] = matrix_(row, col);
            }
            for (size_t dim = 0; dim < 3; dim++)
              param_vector[index++] = translation_[dim];
          }

          void set_matrix (const Matrix<ValueType>& matrix) {
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                 matrix_(row, col) = matrix (row, col);
            }
            compute_offset();
          }

          void get_matrix (Matrix<ValueType>& matrix) const {
            for (size_t row = 0; row < 3; row++) {
              for (size_t col = 0; col < 3; col++)
                 matrix(row, col) = matrix_(row, col);
            }
          }

          void set_translation (Vector<ValueType>& translation) {
            translation_ = translation;
            compute_offset();
          }

          void get_translation (Vector<ValueType>& translation) const {
            translation.allocate(3);
            translation[0] = translation_[0];
            translation[1] = translation_[1];
            translation[2] = translation_[2];
          }

          void set_centre (Vector<ValueType>& centre) {
            centre_ = centre;
            compute_offset();
          }

          void get_centre (Vector<ValueType>& centre) const {
            centre.allocate(3);
            centre[0] = centre_[0];
            centre[1] = centre_[1];
            centre[2] = centre_[2];
          }

          size_t size() const {
            return 6;
          }

          void set_optimiser_weights (Vector<ValueType>& optimiser_weights) {
            assert(size() == optimiser_weights.size());
              optimiser_weights_ = optimiser_weights;
          }

          void get_optimiser_weights (Vector<ValueType>& optimiser_weights) const {
            optimiser_weights.allocate (optimiser_weights_.size());
            for (size_t i = 0; i < optimiser_weights_.size(); i++)
              optimiser_weights[i] = optimiser_weights_[i];
          }

        protected:

          void compute_offset () {
            for( size_t i = 0; i < 3; i++ ) {
              offset_[i] = translation_[i] + centre_[i];
              for( size_t j = 0; j < 3; j++ )
                offset_[i] -= matrix_(i, j) * centre_[j];
            }
          }

          void compute_matrix () {
            const ValueType vx = versor_[0];
            const ValueType vy = versor_[1];
            const ValueType vz = versor_[2];
            const ValueType vw = versor_[3];

            const ValueType xx = vx * vx;
            const ValueType yy = vy * vy;
            const ValueType zz = vz * vz;
            const ValueType xy = vx * vy;
            const ValueType xz = vx * vz;
            const ValueType xw = vx * vw;
            const ValueType yz = vy * vz;
            const ValueType yw = vy * vw;
            const ValueType zw = vz * vw;

            matrix_(0,0) = 1.0 - 2.0 * ( yy + zz );
            matrix_(1,1) = 1.0 - 2.0 * ( xx + zz );
            matrix_(2,2) = 1.0 - 2.0 * ( xx + yy );
            matrix_(0,1) = 2.0 * ( xy - zw );
            matrix_(0,2) = 2.0 * ( xz + yw );
            matrix_(1,0) = 2.0 * ( xy + zw );
            matrix_(2,0) = 2.0 * ( xz - yw );
            matrix_(2,1) = 2.0 * ( yz + xw );
            matrix_(1,2) = 2.0 * ( yz - xw );
          }

          Vector<ValueType> versor_;
          Matrix<ValueType> matrix_;
          Vector<ValueType> translation_;
          Vector<ValueType> centre_;
          Vector<ValueType> offset_;
          Vector<ValueType> optimiser_weights_;

      };
      //! @}
    }
  }
}

#endif
