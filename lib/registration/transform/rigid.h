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

#include "registration/transform/base.h"
#include "math/gradient_descent.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {



      FORCE_INLINE Eigen::Quaterniond compose (const Eigen::Quaterniond& x, const Eigen::Quaterniond& y)
      {
        Eigen::Quaterniond q (
          x.w()*y.w() - x.x()*y.x() - x.y()*y.y() - x.z()*y.z(),
          x.w()*y.x() + x.x()*y.w() + x.y()*y.z() - x.z()*y.y(),
          x.w()*y.y() - x.x()*y.z() + x.y()*y.w() + x.z()*y.x(),
          x.w()*y.z() + x.x()*y.y() - x.y()*y.x() + x.z()*y.w());
        q.normalize();
        return q;
      }


      FORCE_INLINE void set_axis (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& axis,
                                  Eigen::Quaterniond &versor)
      {
        const default_type sinangle2 =  axis.norm();
        if (sinangle2 > 1.0)
          throw Exception ("trying to set a versor with magnitude greater than 1.");
        versor.w() = std::sqrt (1.0 - sinangle2 * sinangle2);
        versor.x() = axis[0];
        versor.y() = axis[1];
        versor.z() = axis[2];
      }



      class VersorUpdate {
        public:
          inline bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
                                  const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
                                  const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
                                  default_type step_size) {

            Eigen::Vector3 axis (g[0], g[1], g[2]);
            Eigen::Quaterniond gradient_rotation (Eigen::AngleAxisd(-step_size * axis.norm(), axis));

            Eigen::Vector3 right_part (x[0], x[1], x[2]);
            Eigen::Quaterniond current_rotation;
            set_axis (right_part, current_rotation);

            Eigen::Quaterniond new_rotation = compose (current_rotation, gradient_rotation);
//            std::cout << current_rotation.w() << " " << current_rotation.x() << " " << current_rotation.y() << " " << current_rotation.z() << std::endl;
//            std::cout << gradient_rotation.w() << " " << gradient_rotation.x() << " " << gradient_rotation.y() << " " << gradient_rotation.z() << std::endl;
//            std::cout << new_rotation.w() << " " << new_rotation.x() << " " << new_rotation.y() << " " << new_rotation.z() << std::endl;
//            Eigen::Quaterniond new_rotation2 = current_rotation * gradient_rotation;
//            std::cout << new_rotation2.w() << " " << new_rotation2.x() << " " << new_rotation2.y() << " " << new_rotation2.z() << std::endl;


            newx[0] = new_rotation.x();
            newx[1] = new_rotation.y();
            newx[2] = new_rotation.z();
            newx[3] = x[3] - step_size * g[3];
            newx[4] = x[4] - step_size * g[4];
            newx[5] = x[5] - step_size * g[5];

            bool changed = false;
            for (size_t n = 0; n < 3; ++n) {
              if (fabs(newx[n] - x[n]) > std::numeric_limits<default_type>::epsilon())
                changed = true;
            }
            return changed;
          }
      };




      class VersorUpdateTest {
        public:
          inline bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
                                  const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
                                  Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
                                  default_type step_size) {
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> vx (x.head<4>());
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> vg (g.head<4>());

            default_type dp = vx.dot(vg);
            for (size_t n = 0; n < 4; ++n)
              vg[n] -= dp * vx[n];

            if (!Math::LinearUpdate() (newx, x, g, step_size))
              return false;

            Eigen::Matrix<default_type, Eigen::Dynamic, 1> v (newx.head(4));
            v.colwise().normalize();
            return newx != x;
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
       * This can be done automatically using functions available in  src/registration/transform/initialiser.h
       *
       */
      class Rigid: public Base  {
        public:

          typedef VersorUpdate UpdateType;

          Rigid () : Base (6) {
            for (size_t i = 0; i < 3; i++)
              this->optimiser_weights[i] = 1.0;
            for (size_t i = 3; i < 6; i++)
              this->optimiser_weights[i] = 1.0;
          }

          Eigen::MatrixXd get_jacobian_wrt_params (const Eigen::Vector3& p) const
          {
            Eigen::MatrixXd jacobian (3, 6);
            jacobian.setZero();

            const default_type vw = versor.w();
            const default_type vx = versor.x();
            const default_type vy = versor.y();
            const default_type vz = versor.z();

            const default_type px = p[0] - this->centre[0];
            const default_type py = p[1] - this->centre[1];
            const default_type pz = p[2] - this->centre[2];

            const default_type vxx = vx * vx;
            const default_type vyy = vy * vy;
            const default_type vzz = vz * vz;
            const default_type vww = vw * vw;
            const default_type vxy = vx * vy;
            const default_type vxz = vx * vz;
            const default_type vxw = vx * vw;
            const default_type vyz = vy * vz;
            const default_type vyw = vy * vw;
            const default_type vzw = vz * vw;

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

            return jacobian;
          }

          void set_rotation (const Eigen::Vector3& axis, default_type angle)
          {
            Eigen::Quaterniond tmp (Eigen::AngleAxisd (angle, axis));
            versor = tmp;
            throw Exception("FIXME: matrix-->trasformation not implemented");
            this->matrix = versor.matrix();
            this->compute_offset();
          }


          void set_parameter_vector (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector)
          {
            Eigen::Vector3 axis;
            axis = param_vector.head(3);

            default_type norm =  param_vector.head(3).norm();

            default_type epsilon = 1e-10;
            if (norm >= 1.0 - epsilon)
              axis /= (norm + epsilon * norm);

            set_axis (axis, versor);
            throw Exception("FIXME: matrix-->trasformation not implemented");
            this->matrix = versor.matrix();
            this->translation = param_vector.tail(3);
            this->compute_offset();
            #ifndef NONSYMREGISTRATION
              this->compute_halfspace_transformations(); // TODO
              throw Exception("rigid symmetric registration not implemented yet");
            #endif

            std::cout << param_vector.transpose() << std::endl;
            std::cout << versor.w() << " " << versor.x() << " " << versor.y() << " " << versor.z() << std::endl;
            std::cout << this->matrix << std::endl<< std::endl;
            std::cout <<this->translation<< std::endl<< std::endl;
            std::cout <<this->offset<< std::endl<< std::endl << std::endl;

          }

          void get_parameter_vector (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector) const {
            param_vector.resize(6);
            param_vector[0] = versor.x();
            param_vector[1] = versor.y();
            param_vector[2] = versor.z();
            param_vector.tail(3) = this->translation;
          }

          UpdateType* get_gradient_descent_updator (){
            return &gradient_descent_updator;
          }

        protected:
          Eigen::Quaterniond versor;
          UpdateType gradient_descent_updator;
      };




//      /*! A 3D rigid transformation class for registration.
//       *
//       * This class defines a rigid transform using 6 parameters. The first 3 parameters define rotation using a versor (unit quaternion),
//       * while the last 3 parameters define translation. Note that since the versor parameters do not define a vector space, any updates
//       * must be performed using a versor composition (not an addition). This can be achieved by passing the update_parameters method
//       * from this class as a function pointer to the gradient_decent run method.
//       *
//       * This class supports the ability to define the centre of rotation. This should be set prior to commencing registration based on
//       * the centre of the target image. The translation also should be initialised as moving image centre minus the target image centre.
//       * This can done automatically using functions available in  src/registration/transform/initialiser.h
//       *
//       */
//      template <typename default_type = float>
//      class RigidTest : public Base<default_type>  {
//        public:

//          typedef typename Base<default_type>::ParameterType ParameterType;
//          typedef VersorUpdateTest UpdateType;

//          RigidTest () : Base<default_type> (7) {
//            this->optimiser_weights_ = 1.0;
//          }

//          template <class PointType>
//          void get_jacobian_wrt_params (const PointType& p, Matrix<default_type>& jacobian) const {

//            jacobian.resize(3,7);
//            jacobian.zero();
//            Vector<default_type> v (3);
//            v[0] = p[0] - this->centre_[0];
//            v[1] = p[1] - this->centre_[1];
//            v[2] = p[2] - this->centre_[2];

//              // compute Jacobian with respect to quaternion parameters
//            jacobian(0,1) =  2.0 * ( versor_[1] * v[0] + versor_[2] * v[1] + versor_[3] * v[2]);
//            jacobian(0,2) =  2.0 * (-versor_[2] * v[0] + versor_[1] * v[1] + versor_[0] * v[2]);
//            jacobian(0,3) =  2.0 * (-versor_[3] * v[0] - versor_[0] * v[1] + versor_[1] * v[2]);
//            jacobian(0,0) = -2.0 * (-versor_[0] * v[0] + versor_[3] * v[1] - versor_[2] * v[2]);

//            jacobian(1,1) = -jacobian(0,2);
//            jacobian(1,2) =  jacobian(0,1);
//            jacobian(1,3) =  jacobian(0,0);
//            jacobian(1,0) = -jacobian(0,3);

//            jacobian(2,1) = -jacobian(0,3);
//            jacobian(2,2) = -jacobian(0,0);
//            jacobian(2,3) =  jacobian(0,1);
//            jacobian(2,0) =  jacobian(0,2);

//             // compute derivatives for the translation part
//            for (size_t dim = 0; dim < 3; ++dim)
//              jacobian(dim, 4+dim) = 1.0;
//          }

//          void set_rotation (const Math::Vector<default_type>& axis, default_type angle) {
//            versor_.set(axis, angle);
//            compute_matrix();
//            this->compute_offset();
//          }


//          void set_parameter_vector (const Math::Vector<default_type>& param_vector) {
//            versor_ = Versor<default_type> (param_vector.sub(0,4));
//            compute_matrix();
//            this->translation_ = param_vector.sub(4,7);
//            this->compute_offset();
//          }

//          void get_parameter_vector (Vector<default_type>& param_vector) const {
//            param_vector.allocate (7);
//            param_vector[0] = versor_[0];
//            param_vector[1] = versor_[1];
//            param_vector[2] = versor_[2];
//            param_vector[3] = versor_[3];
//            param_vector[4] = this->translation_[0];
//            param_vector[5] = this->translation_[1];
//            param_vector[6] = this->translation_[2];
//          }

//          UpdateType* get_gradient_descent_updator (){
//            return &gradient_descent_updator;
//          }

//        protected:

//          void compute_matrix () {
//            versor_.to_matrix (this->matrix_);
//          }

//          Versor<default_type> versor_;
//          UpdateType gradient_descent_updator;
//      };

      //! @}
    }
  }
}

#endif
