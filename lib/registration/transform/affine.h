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
#include <algorithm> // std::min_element
#include <iterator>

using namespace MR::Math;

namespace MR
{
  namespace Math
  {

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
            DEBUG("step size: " + str(step_size));
            assert (newx.size() == 12);
            assert (x.size() == 12);
            assert (g.size() == 12);

            Eigen::Matrix<ValueType, 12, 1> delta;
            Eigen::Matrix<ValueType, 4, 4> X, Delta, G, A, Asqrt, B, Bsqrt, Bsqrtinv, Xnew;

            // enforce updates in the range of small angle updates
            Registration::Transform::param_vec2mat(g, G);
            if (step_size > 0.1 / G.block(0,0,3,3).array().abs().maxCoeff())
              step_size = 0.1 / G.block(0,0,3,3).array().abs().maxCoeff();

            Registration::Transform::param_vec2mat(x, X);
            // reduce step size if determinant of matrix is negative (happens rarely at first few iterations)
            size_t cnt = 0;
            default_type factor = 0.9;
            while (true) {
              delta = g * step_size;
              Registration::Transform::param_vec2mat(delta, Delta);

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
            Registration::Transform::param_mat2vec(Xnew, newx);
#ifdef REGISTRATION_GRADIENT_DESCENT_DEBUG
            if (newx.isApprox(x)){
              ValueType debug = 0;
              for (ssize_t i=0; i<newx.size(); ++i){
                debug += std::abs(newx[i]-x[i]);
              }
              INFO("affine update parameter cumulative change: " + str(debug));
              VEC(newx);
              VEC(g);
              VAR(step_size);
            }
#endif
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
          // typedef Math::AffineLinearNonSymmetricUpdate UpdateType;
          typedef Math::AffineUpdate UpdateType;
          typedef AffineRobustEstimator RobustEstimatorType;
          typedef int has_robust_estimator;

          Affine () : Base (12) {
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

          template <class ParamType, class VectorType>
          bool robust_estimate (
            VectorType& gradient,
            std::vector<VectorType>& grad_estimates,
            const ParamType& params,
            const VectorType& parameter_vector,
            const default_type& weiszfeld_precision = 1.0e-6,
            const size_t& weiszfeld_iterations = 1000,
            default_type learning_rate = 1.0) const {
            assert (gradient.size()==12);
            size_t n_estimates = grad_estimates.size();
            assert (n_estimates>1);

            Eigen::Matrix<ParameterType, 4, 4> X, X_upd;
            param_vec2mat (parameter_vector, X);

            // get tetrahedron
            const size_t n_vertices = 4;
            const Eigen::Matrix<ParameterType, 3, n_vertices> vertices = params.control_points;
            Eigen::Matrix<ParameterType, 4, n_vertices> vertices_4;
            vertices_4.block(0, 0, 3, n_vertices) = vertices;
            vertices_4.row(3) << 1.0, 1.0, 1.0, 1.0;

            // transform each vertex with each candidate transformation
            // weighting

            if (this->optimiser_weights.size()){
              assert (this->optimiser_weights.size() == 12);
              for (size_t j =0; j < n_estimates; ++j){
                grad_estimates[j].array() *= this->optimiser_weights.array();
              }
            }
            // let's go to the small angle regime
            for (size_t j =0; j < n_estimates; ++j){
              if (grad_estimates[j].head(9).cwiseAbs().maxCoeff() * learning_rate > 0.2){
                learning_rate = 0.2 / grad_estimates[j].head(9).cwiseAbs().maxCoeff();
              }
            }

            std::vector<Eigen::Matrix<ParameterType, 3, Eigen::Dynamic>> transformed_vertices(4);
            for (auto& vert : transformed_vertices){
              vert.resize (3, n_estimates);
            }
            transform_type trafo_upd;
            // adjust learning rate if neccessary
            size_t max_iter = 10000;
            while (max_iter > 0) {
              bool learning_rate_ok = true;
              for (size_t j =0; j < n_estimates; ++j){
                Eigen::Matrix<ParameterType, Eigen::Dynamic, 1> candidate = (parameter_vector - learning_rate * grad_estimates[j]);
                assert (is_finite (candidate));
                param_vec2mat (candidate, trafo_upd.matrix());
                if (trafo_upd.matrix().block (0,0,3,3).determinant() < 0){
                  learning_rate *= 0.1;
                  learning_rate_ok = false;
                  break;
                }
                for (size_t i = 0; i < n_vertices; ++i){
                  transformed_vertices[i].col(j) = trafo_upd * vertices.col(i);
                }
              }
              if (learning_rate_ok) break;
              --max_iter;
            }
            if (max_iter != 10000) INFO ("affine robust estimate learning rate adjusted to " + str (learning_rate));

            // compute vertex-wise median
            Eigen::Matrix<ParameterType, 4, n_vertices> vertices_transformed_median;
            for (size_t i = 0; i < n_vertices; ++i){
              Eigen::Matrix<ParameterType, 3, 1> median_vertex;
              Math::median_weiszfeld (transformed_vertices[i], median_vertex, weiszfeld_iterations, weiszfeld_precision);
              vertices_transformed_median.col(i) << median_vertex, 1.0;
            }

            // get median transformation from median_vertices = trafo * vertices
            Eigen::ColPivHouseholderQR<Eigen::Matrix<ParameterType, 4, n_vertices>> dec(vertices_4.transpose());
            Eigen::Matrix<ParameterType, 4, 4> trafo_median;
            trafo_median.transpose() = dec.solve(vertices_transformed_median.transpose());

            // undo weighting and convert transformation matrix to gradient update vector
            VectorType x_new;
            x_new.resize(12);
            param_mat2vec(trafo_median, x_new);
            x_new -= parameter_vector;
            if (this->optimiser_weights.size()){
              x_new.array() /= this->optimiser_weights.array();
            }
            // revert learning rate and multiply by number of estimates to mimic sum of gradients
            // this corresponds to the sum of the projections of the gradient estimates onto the median
            gradient.array() = - x_new.array() * (default_type(n_estimates) / learning_rate);
            assert(is_finite(gradient));
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
