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
#include "math/median.h"

using namespace MR::Math;

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{
    template<class MatrixType = Eigen::Matrix<default_type, 4, 4>>
    void param_mat2vec_affine (const MatrixType& transformation_matrix,
                                      Eigen::Matrix<default_type,
                                      Eigen::Dynamic, 1>& param_vector) {
        assert(transformation_matrix.cols()==4);
        assert(transformation_matrix.rows()>=3);
        size_t index = 0;
        assert(param_vector.size() == 12);
        for (size_t row = 0; row < 3; ++row) {
          for (size_t col = 0; col < 3; ++col)
            param_vector[index++] = transformation_matrix (row, col);
        }
        for (size_t dim = 0; dim < 3; ++dim)
          param_vector[index++] = transformation_matrix(dim,3);
      }

    template<class MatrixType = Eigen::Matrix<default_type, 4, 4>>
    void param_vec2mat_affine (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& param_vector,
                                     MatrixType& transformation_matrix) {
        assert(transformation_matrix.cols()==4);
        assert(transformation_matrix.rows()>=3);
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

            param_vec2mat_affine(x, X);
            // reduce step size if determinant of matrix is negative (happens rarely at first few iterations)
            size_t cnt = 0;
            default_type factor = 0.9;
            while (true) {
              delta = g * step_size;
              param_vec2mat_affine(delta, Delta);

              A = X - Delta;
              A(3,3) = 1.0;
              if (A.determinant() < 0) {
                step_size *= factor;
                ++cnt;
              } else {
                break;
              }
            }
            if (cnt > 0) INFO("affine: gradient descent step size was too large. Multiplied by factor "
             + str(std::pow (factor, cnt), 4) + " (now: "+ str(step_size, 4) + ")");

            Asqrt = A.sqrt().eval();
            assert(A.isApprox(Asqrt * Asqrt));
            B = X.inverse() + Delta;
            B(3,3) = 1.0;
            assert(B.determinant() > 0.0);
            Bsqrt = B.sqrt().eval();
            Bsqrtinv = Bsqrt.inverse().eval();
            assert(B.isApprox(Bsqrt * Bsqrt));

            // approximation for symmetry reasons as
            // A and B don't commute
            Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
            param_mat2vec_affine(Xnew, newx);
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
            assert (gradient.size()==12);
            size_t n_estimates = grad_estimates.size();
            assert (n_estimates>1);
            const size_t n_corners = 4;
            Eigen::Matrix<default_type, 3, n_corners> corners;
            Eigen::Matrix<default_type, 4, n_corners> corners_4;
            Eigen::Matrix<default_type, 4, n_corners> corners_transformed_median;
            corners.col(0) << 1.0,    0, -Math::sqrt1_2;
            corners.col(1) <<-1.0,    0, -Math::sqrt1_2;
            corners.col(2) <<   0,  1.0,  Math::sqrt1_2;
            corners.col(3) <<   0, -1.0,  Math::sqrt1_2;
            corners *= 10.0;


            std::vector<Eigen::Matrix<default_type, 3, Eigen::Dynamic>> transformed_corner(4);
            for (auto& corner : transformed_corner){
              corner.resize(3,n_estimates);
            }

            Eigen::Matrix<default_type, 4, 4> X, X_upd;
            Math::param_vec2mat_affine(parameter_vector, X);

            // weighting
            // for (size_t j =0; j < n_estimates; ++j){
            //   for (size_t i = 0; i<this->optimiser_weights.size(); ++i){
            //     grad_estimates[j][i] *= this->optimiser_weights[i];
            //   }
            // }

            transform_type trafo_upd;
            for (size_t j =0; j < n_estimates; ++j){
              // gradient += grad_estimates[j]; // TODO remove me
              Eigen::Matrix<default_type, Eigen::Dynamic, 1> candidate =  parameter_vector - grad_estimates[j] / grad_estimates[j].norm();
              Math::param_vec2mat_affine(candidate, trafo_upd.matrix());
              for (size_t i = 0; i < n_corners; ++i){
                transformed_corner[i].col(j) = trafo_upd * corners.col(i);
              }
            }
            // return true; // hack

            for (size_t i = 0; i < n_corners; ++i){
              Eigen::Matrix<default_type, 3, 1> median_corner;
              Math::geometric_median3 (transformed_corner[i], median_corner);
              corners_transformed_median.col(i) << median_corner, 1.0;
              corners_4.col(i) << corners.col(i), 1.0;
            }
            Eigen::ColPivHouseholderQR<Eigen::Matrix<default_type, 4, n_corners>> dec(corners_4.transpose());
            Eigen::Matrix<default_type,4,4> trafo_median;
            trafo_median.transpose() = dec.solve(corners_transformed_median.transpose());
            VectorType x_new;
            x_new.resize(12);
            Math::param_mat2vec_affine(trafo_median, x_new);
            gradient = parameter_vector - x_new;
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
