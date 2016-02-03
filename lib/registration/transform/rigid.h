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
#include "types.h"
#include "math/median.h"
#include <algorithm> // std::min_element
#include <iterator>

using namespace MR::Math;

namespace MR
{
  namespace Math
  {

    class RigidLinearNonSymmetricUpdate {
      public:
        template <typename ValueType>
          inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g,
              ValueType step_size) {
            assert (newx.size() == x.size());
            assert (g.size() == x.size());
            newx = x - step_size * g;
            Eigen::Matrix<ValueType, 4, 4> X;
            Registration::Transform::param_vec2mat(newx, X);
            Eigen::Matrix<ValueType, 3, 3> L(X.template block<3,3>(0,0));
            Eigen::Matrix<ValueType, 3, 3> R;
            project_linear2rotation(L, R);
            X.template block<3,3>(0,0) = R;
            Registration::Transform::param_mat2vec(X, newx);
            return !(newx.isApprox(x));
          }

      void set_control_points (
        const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& points,
        const Eigen::Vector3d& coherence_dist,
        const Eigen::Vector3d& stop_length,
        const Eigen::Vector3d& voxel_spacing ) {
        assert(points.rows() == 4);
        assert(points.cols() == 4);
        control_points = points;
        coherence_distance = coherence_dist;
        stop_len << stop_length, 0.0;
        spacing << voxel_spacing, 1.0;
      }

      private:
        Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> control_points;
        Eigen::Vector3d coherence_distance;
        Eigen::Matrix<default_type, 4, 1> stop_len, spacing;

        template<class ValueType>
        void project_linear2rotation(const Eigen::Matrix<ValueType, 3, 3>& linear, Eigen::Matrix<ValueType, 3, 3>& rotation) const
        {
          Eigen::JacobiSVD<Eigen::Matrix<ValueType, 3, 3>> svd(linear, Eigen::ComputeFullU | Eigen::ComputeFullV);

          ValueType x = (svd.matrixU() * svd.matrixV().adjoint()).determinant(); // so x has absolute value 1
          Eigen::Matrix<ValueType, 3, 1> sv(svd.singularValues());
          sv.coeffRef(0) *= x;
          Eigen::Matrix<ValueType, 3, 3> m(svd.matrixU());
          m.col(0) /= x;
          rotation = m * svd.matrixV().adjoint();
          assert((rotation * rotation.transpose().eval()).isApprox(Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic>::Identity(3,3)) && "orthonormal?");
        }
    };

//     class RigidUpdate {
//       public:
//         template <typename ValueType>
//           inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx,
//               const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
//               const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g,
//               ValueType step_size) {
//             DEBUG("step size: " + str(step_size));
//             assert (newx.size() == 12);
//             assert (x.size() == 12);
//             assert (g.size() == 12);

//             Eigen::Matrix<ValueType, 12, 1> delta;
//             Eigen::Matrix<ValueType, 4, 4> X, Delta, G, A, Asqrt, B, Bsqrt, Bsqrtinv, Xnew;

//             // enforce updates in the range of small angle updates
//             Registration::Transform::param_vec2mat(g, G);
//             if (step_size > 0.1 / G.block(0,0,3,3).array().abs().maxCoeff())
//               step_size = 0.1 / G.block(0,0,3,3).array().abs().maxCoeff();

//             Registration::Transform::param_vec2mat(x, X);
//             // reduce step size if determinant of matrix is negative (happens rarely at first few iterations)
//             size_t cnt = 0;
//             default_type factor = 0.9;
//             while (true) {
//               delta = g * step_size;
//               Registration::Transform::param_vec2mat(delta, Delta);

//               A = X - Delta;
//               A(3,3) = 1.0;
//               if (A.determinant() < 0) {
//                 step_size *= factor;
//                 ++cnt;
//               } else {
//                 break;
//               }
//             }
//             if (cnt > 0) INFO("rigid: gradient descent step size was too large. Multiplied by factor "
//              + str(std::pow (factor, cnt), 4) + " (now: "+ str(step_size, 4) + ")");

//             Asqrt = A.sqrt().eval();
//             assert(A.isApprox(Asqrt * Asqrt));
//             B = X.inverse() + Delta;
//             B(3,3) = 1.0;
//             assert(B.determinant() > 0.0);
//             Bsqrt = B.sqrt().eval();
//             Bsqrtinv = Bsqrt.inverse().eval();
//             assert(B.isApprox(Bsqrt * Bsqrt));

//             // approximation for symmetry reasons as
//             // A and B don't commute
//             Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
//             Registration::Transform::param_mat2vec(Xnew, newx);
// #ifdef REGISTRATION_GRADIENT_DESCENT_DEBUG
//             if (newx.isApprox(x)){
//               ValueType debug = 0;
//               for (ssize_t i=0; i<newx.size(); ++i){
//                 debug += std::abs(newx[i]-x[i]);
//               }
//               INFO("rigid update parameter cumulative change: " + str(debug));
//               VEC(newx);
//               VEC(g);
//               VAR(step_size);
//             }
// #endif
//             return !(newx.isApprox(x));
//           }
//     };
  }

  namespace Registration
  {
    namespace Transform
    {

      /** \addtogroup Transforms
      @{ */

      /*! A 3D rigid transformation class for registration.
       *
       * This class supports the ability to define the centre of rotation.
       * This should be set prior to commencing registration based on the centre of the target image.
       * The translation also should be initialised as moving image centre minus the target image centre.
       *
       */
      class Rigid : public Base  {
        public:

          typedef typename Base::ParameterType ParameterType;
          typedef Math::RigidLinearNonSymmetricUpdate UpdateType;
          // typedef Math::RigidUpdate UpdateType;
          // typedef RigidRobustEstimator RobustEstimatorType;

          Rigid () : Base (12) {
            const Eigen::Vector4d weights (0.0003, 0.0003, 0.0003, 1.0);
            this->optimiser_weights << weights, weights, weights;
          }

          Eigen::Matrix<default_type, 4, 1> get_jacobian_vector_wrt_params (const Eigen::Vector3& p) const {
            Eigen::Matrix<default_type, 4, 1> jac;
            jac.head(3) = p - centre;
            jac(3) = 1.0;
            return jac;
          }

          Eigen::MatrixXd get_jacobian_wrt_params (const Eigen::Vector3& p) const {
            Eigen::MatrixXd jacobian (3, 12);
            jacobian.setZero();
            const auto v = get_jacobian_vector_wrt_params(p);
            jacobian.block<1, 4>(0, 0) = v;
            jacobian.block<1, 4>(1, 4) = v;
            jacobian.block<1, 4>(2, 8) = v;
            return jacobian;
          }

          void set_parameter_vector (const Eigen::Matrix<ParameterType, Eigen::Dynamic, 1>& param_vector) {
            this->trafo.matrix() = Eigen::Map<const Eigen::Matrix<ParameterType, 3, 4, Eigen::RowMajor> >(&param_vector(0));
            this->compute_halfspace_transformations();
          }

          void get_parameter_vector (Eigen::Matrix<ParameterType, Eigen::Dynamic, 1>& param_vector) const {
            param_vector.resize (12);
            param_mat2vec (this->trafo.matrix(), param_vector);
          }

          UpdateType* get_gradient_descent_updator () {
            return &gradient_descent_updator;
          }

        protected:
          UpdateType gradient_descent_updator;
      };
      //! @}
    }
  }
}

#endif
