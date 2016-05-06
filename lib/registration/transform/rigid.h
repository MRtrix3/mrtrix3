/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __registration_transform_rigid_h__
#define __registration_transform_rigid_h__

#include "registration/transform/base.h"
#include "types.h"
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
  }

  namespace Registration
  {
    namespace Transform
    {

      /** \addtogroup Transforms
      @{ */

      /*! A 3D rigid transformation class for registration.
       *
       */
      class Rigid : public Base  {
        public:

          typedef typename Base::ParameterType ParameterType;
          typedef Math::RigidLinearNonSymmetricUpdate UpdateType;

          Rigid () : Base (12) {
            const Eigen::Vector4d weights (0.0003, 0.0003, 0.0003, 1.0);
            this->optimiser_weights << weights, weights, weights;
          }

          Eigen::Matrix<default_type, 4, 1> get_jacobian_vector_wrt_params (const Eigen::Vector3& p) const {
            Eigen::Matrix<default_type, 4, 1> jac;
            jac.head(3) = p - this->centre;
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
