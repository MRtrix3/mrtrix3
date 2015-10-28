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

#include "registration/transform/base.h"
#include "math/gradient_descent.h"
#include "types.h"

using namespace MR::Math;

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{

    void matrix_to_parameter_vector (const Eigen::Matrix<default_type, 4, 4>& transformation_matrix,
       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector) {
        size_t index = 0;
        assert(param_vector.size() == 12);
        for (size_t row = 0; row < 3; ++row) {
          for (size_t col = 0; col < 3; ++col)
            param_vector[index++] = transformation_matrix (row, col);
        }
        for (size_t dim = 0; dim < 3; ++dim)
          param_vector[index++] = transformation_matrix(dim,3);
      }

    void parameter_vector_to_matrix (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector,
        Eigen::Matrix<default_type, 4, 4>& transformation_matrix) {
        size_t index = 0;
        transformation_matrix.setIdentity();
        for (size_t row = 0; row < 3; ++row) {
          for (size_t col = 0; col < 3; ++col)
            transformation_matrix (row, col) = param_vector[index++];
        }
        for (size_t dim = 0; dim < 3; ++dim)
          transformation_matrix(dim,3) = param_vector[index++];
        // transformation_matrix(3,3) = 1.0;
        // M1.template transpose().template topLeftCorner<3,3>() << newx1.template segment<9>(0);
        // M1.col(3) << newx1.template segment<3>(9), 1.0;
        // M2.template transpose().template topLeftCorner<3,3>() << newx2.template segment<9>(0);
        // M2.col(3) << newx2.template segment<3>(9), 1.0;
      }

    class AffineLinearNonSymmetricUpdate {
      public:
        template <typename ValueType>
          inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g,
              ValueType step_size) {
            assert (newx.size() == x.size());
            assert (g.size() == x.size());
            newx = x - step_size * g;
            return !(newx.isApprox(x));
          }
    };

    class AffineUpdate {
      public:
        template <typename ValueType>
          inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g,
              ValueType step_size) {
            assert (newx.size() == 12);
            assert (x.size() == 12);
            assert (g.size() == 12);

            Eigen::Matrix<ValueType, 12, 1> delta;
            Eigen::Matrix<ValueType, 4, 4> X, Delta, A, Asqrt, B, Bsqrt, Bsqrtinv, Xnew;

            parameter_vector_to_matrix(x, X);
            delta = g * step_size;
            parameter_vector_to_matrix(delta, Delta);

            A = X - Delta;
            A(3,3) = 1.0;
            Asqrt = A.sqrt();
            assert(A.isApprox(Asqrt * Asqrt));
            B = X.inverse() + Delta;
            B(3,3) = 1.0;
            Bsqrt = B.sqrt();
            Bsqrtinv = Bsqrt.inverse();
            assert(B.isApprox(Bsqrt * Bsqrt));

            // approximation for symmetry reasons as
            // A and B don't commute
            Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
            matrix_to_parameter_vector(Xnew, newx);
            // if (verbose){
            //   Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", ";\n", "", "", "", "");
            //   WARN("AffineUpdate: newx " + str(newx.transpose(),12));
            //   VAR(Xnew.sqrt().format(fmt));
            //   VAR(Xnew.sqrt().inverse().format(fmt));
            // }
            return !(newx.isApprox(x));
          }
    };
  }

  namespace Registration
  {
    namespace Transform
    {
      class AffineRobustEstimator {
      public:
        template <typename ValueType>
          inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g,
              ValueType step_size) {
            assert (newx.size() == x.size());
            assert (g.size() == x.size());
            newx = x - step_size * g;
            return !(newx.isApprox(x));
          }
    };

      /** \addtogroup Transforms
      @{ */

      /*! A 3D affine transformation class for registration.
       *
       * This class supports the ability to define the centre of rotation.
       * This should be set prior to commencing registration based on the centre of the target image.
       * The translation also should be initialised as moving image centre minus the target image centre.
       *
       */
      class Affine : public Base  {
        public:

          typedef typename Base::ParameterType ParameterType;
          typedef Math::AffineUpdate UpdateType;
          typedef AffineRobustEstimator RobustEstimatorType;
          typedef int has_robust_estimator;
          // void has_robust_estimator() { };

          Affine () : Base (12) {
            for (size_t i = 0; i < 9; ++i)
              this->optimiser_weights[i] = 0.003;
            for (size_t i = 9; i < 12; ++i)
              this->optimiser_weights[i] = 1.0;
          }


          Eigen::MatrixXd get_jacobian_wrt_params (const Eigen::Vector3& p) const {
            Eigen::MatrixXd jacobian (3, 12);
            jacobian.setZero();
            Eigen::Vector3 v;
            v[0] = p[0] - this->centre[0];
            v[1] = p[1] - this->centre[1];
            v[2] = p[2] - this->centre[2];
            size_t blockOffset = 0;
            for (size_t block = 0; block < 3; ++block) {
              for (size_t dim = 0; dim < 3; ++dim)
                jacobian (block, blockOffset + dim) = v[dim];
              blockOffset += 3;
            }
            for (size_t dim = 0; dim < 3; ++dim) {
              jacobian (dim, blockOffset + dim) = 1.0;
            }
            return jacobian;
          }


          void set_parameter_vector (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector) {
            size_t index = 0;
            for (size_t row = 0; row < 3; ++row) {
              for (size_t col = 0; col < 3; ++col)
                this->trafo (row, col) = param_vector[index++];
            }
            for (size_t dim = 0; dim < 3; ++dim)
              this->trafo(dim,3) = param_vector[index++];
            this->compute_halfspace_transformations();
          }

          void get_parameter_vector (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector) const {
            param_vector.resize (12);
            size_t index = 0;
            for (size_t row = 0; row < 3; ++row) {
              for (size_t col = 0; col < 3; ++col)
                param_vector[index++] = this->trafo (row, col);
            }
            for (size_t dim = 0; dim < 3; ++dim)
              param_vector[index++] = this->trafo(dim,3);
          }

          UpdateType* get_gradient_descent_updator () {
            return &gradient_descent_updator;
          }

          template <class ParamType, class VectorType>
          bool robust_estimate(
            VectorType& gradient,
            std::vector<VectorType>& grad_estimates,
            const ParamType& params,
            const VectorType& parameter_vector) const {
            for (auto& grad_estimate : grad_estimates ){
              gradient += grad_estimate;
            }
            return true;
          }

        protected:
          UpdateType gradient_descent_updator;
          RobustEstimatorType robust_estimator;
      };
      //! @}
    }
  }
}

#endif
