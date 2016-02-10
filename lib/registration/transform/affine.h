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

#ifndef __registration_transform_affine_h__
#define __registration_transform_affine_h__

#include "registration/transform/base.h"
#include "types.h"
#include "math/math.h"
#include "math/median.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      class AffineLinearNonSymmetricUpdate {
        public:
          bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
              default_type step_size) {
            assert (newx.size() == x.size());
            assert (g.size() == x.size());
            newx = x - step_size * g;
            return !(newx.isApprox(x));
        }
      };

      class AffineUpdate {
        public:
          bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
              default_type step_size);

          void set_control_points (
            const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& points,
            const Eigen::Vector3d& coherence_dist,
            const Eigen::Vector3d& stop_length,
            const Eigen::Vector3d& voxel_spacing );

        private:
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> control_points;
          Eigen::Vector3d coherence_distance;
          Eigen::Matrix<default_type, 4, 1> stop_len, recip_spacing;
      };

      class AffineRobustEstimator {
        public:
          inline bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
              default_type step_size);
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
          typedef AffineUpdate UpdateType;
          // typedef Math::ParameterUpdateCheck UpdateCheckType;
          typedef AffineRobustEstimator RobustEstimatorType;
          typedef int has_robust_estimator;

          Affine () : Base (12) {
            const Eigen::Vector4d weights (0.0003, 0.0003, 0.0003, 1.0);
            this->optimiser_weights << weights, weights, weights;
          }

          Eigen::Matrix<default_type, 4, 1> get_jacobian_vector_wrt_params (const Eigen::Vector3& p) const ;

          Eigen::MatrixXd get_jacobian_wrt_params (const Eigen::Vector3& p) const ;

          void set_parameter_vector (const Eigen::Matrix<ParameterType, Eigen::Dynamic, 1>& param_vector);

          void get_parameter_vector (Eigen::Matrix<ParameterType, Eigen::Dynamic, 1>& param_vector) const;

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
