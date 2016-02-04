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
            assert (newx.size() == 12);
            assert (x.size() == 12);
            assert (g.size() == 12);

            Eigen::Matrix<ValueType, 12, 1> delta;
            Eigen::Matrix<ValueType, 4, 4> X, Delta, G, A, Asqrt, B, Bsqrt, Bsqrtinv, Xnew, P, Diff;
            Registration::Transform::param_vec2mat(g, G);
            Registration::Transform::param_vec2mat(x, X);

            // enforce updates in the range of small angles
            if (step_size * G.block(0,0,3,3).array().abs().maxCoeff() > 0.2) {
              step_size = 0.2 / G.block(0,0,3,3).array().abs().maxCoeff();
            }
            // use control points and coherence length as update criterion
            if (control_points.size()) {
              P = control_points;
              const ValueType orig_step_size(step_size);
              const ValueType step_down_factor(0.5);
              while (true) {
                delta = g * step_size;
                Registration::Transform::param_vec2mat(delta, Delta);
                if ((X+Delta).determinant() <= 0.0){ step_size *= step_down_factor; continue; }
                Diff.noalias() = ((X+Delta) * P - X * P).cwiseAbs();
                if ((Diff.template block<3,1>(0,0) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
                if ((Diff.template block<3,1>(0,1) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
                if ((Diff.template block<3,1>(0,2) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
                if ((Diff.template block<3,1>(0,3) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}

                A = X - Delta;
                A(3,3) = 1.0;
                if (A.determinant() <= 0.0){ step_size *= step_down_factor; continue; }

                B = X.inverse() + Delta;
                B(3,3) = 1.0;
                if (B.determinant() <= 0.0){ step_size *= step_down_factor; continue; }

                Asqrt = A.sqrt().eval();
                assert(A.isApprox(Asqrt * Asqrt));
                Bsqrt = B.sqrt().eval();
                assert(B.isApprox(Bsqrt * Bsqrt));
                Bsqrtinv = Bsqrt.inverse().eval();

                Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
                Diff.noalias() = ((Xnew) * P - X * P).cwiseAbs();
                if ((Diff.template block<3,1>(0,0) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
                if ((Diff.template block<3,1>(0,1) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
                if ((Diff.template block<3,1>(0,2) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
                if ((Diff.template block<3,1>(0,3) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}

                break;
              }
              if (orig_step_size != step_size) { DEBUG("step size changed from " + str(orig_step_size) + " to " + str(step_size)); }
            } else {
              // reduce step size if determinant of matrix becomes negative (happens rarely at first few iterations)
              size_t cnt = 0;
              const default_type factor(0.9);
              while (true) {
                delta = g * step_size;
                Registration::Transform::param_vec2mat(delta, Delta);
                if (Delta.block(0,0,3,3).array().abs().maxCoeff() > 0.1) {
                  step_size = 0.09 / G.block(0,0,3,3).array().abs().maxCoeff();
                  INFO(str(step_size) + " " + str(g * step_size));
                  continue;
                }
                if (Delta.block(0,3,3,1).array().abs().maxCoeff() > 10.0){
                  step_size = 9.0 / G.block(0,3,3,1).array().abs().maxCoeff();
                  INFO(str(step_size) + " " + str(g * step_size));
                  continue;
                }
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

              B = X.inverse() + Delta;
              B(3,3) = 1.0;
              assert(B.determinant() > 0.0);
              Asqrt = A.sqrt().eval();
              assert(A.isApprox(Asqrt * Asqrt));
              Bsqrt = B.sqrt().eval();
              assert(B.isApprox(Bsqrt * Bsqrt));
              Bsqrtinv = Bsqrt.inverse().eval();

              // approximation for symmetry reasons as
              // A and B don't commute
              Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
            }


            // MAT(X);
            // MAT(Xnew);
            // MAT(X.inverse());
            // MAT(Delta);
            // MAT(Asqrt);
            // MAT(Bsqrtinv);
            Registration::Transform::param_mat2vec(Xnew, newx);

            // stop criterion based on max shift of control points
            if (control_points.size()) {
              Diff.row(0) *= recip_spacing(0);
              Diff.row(1) *= recip_spacing(1);
              Diff.row(2) *= recip_spacing(2);
              Diff.colwise() -= stop_len;
              // MAT(Diff);
              if (Diff.template block<3,4>(0,0).maxCoeff() <= 0.0) {
                DEBUG("max control point movement (" + str(Diff.template block<3,4>(0,0).maxCoeff()) +
                ") smaller than tolerance" );
                return false;
              }
            }
// #ifdef REGISTRATION_GRADIENT_DESCENT_DEBUG
//             if (newx.isApprox(x)){
//               ValueType debug = 0;
//               for (ssize_t i=0; i<newx.size(); ++i){
//                 debug += std::abs(newx[i]-x[i]);
//               }
//               INFO("affine update parameter cumulative change: " + str(debug));
//               VEC(newx);
//               VEC(g);
//               VAR(step_size);
//             }
// #endif
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
            recip_spacing << voxel_spacing.cwiseInverse(), 1.0;
          }

        private:
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> control_points;
          Eigen::Vector3d coherence_distance;
          Eigen::Matrix<default_type, 4, 1> stop_len, recip_spacing;
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
          // typedef Math::ParameterUpdateCheck UpdateCheckType;
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
            const Eigen::Matrix<ParameterType, 4, n_vertices> vertices_4 = params.control_points;
            const Eigen::Matrix<ParameterType, 3, n_vertices> vertices = vertices_4.template block<3,4>(0,0);

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
          // UpdateCheckType gradient_descent_update_check;
          RobustEstimatorType robust_estimator;
      };
      //! @}
    }
  }
}

#endif
