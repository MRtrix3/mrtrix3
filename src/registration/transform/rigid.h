/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __registration_transform_rigid_h__
#define __registration_transform_rigid_h__

#include "registration/transform/base.h"
#include "types.h"
#include "math/math.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      class RigidLinearNonSymmetricUpdate { MEMALIGN (RigidLinearNonSymmetricUpdate)
        public:
          RigidLinearNonSymmetricUpdate ( ):
            use_convergence_check (false) {  }

          bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
              default_type step_size);

          void set_control_points (
            const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& points,
            const Eigen::Vector3d& coherence_dist,
            const Eigen::Vector3d& stop_length,
            const Eigen::Vector3d& voxel_spacing );

          void set_convergence_check (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope_threshold,
                                      default_type alpha,
                                      default_type beta,
                                      size_t buffer_len,
                                      size_t min_iter) {
            // convergence check using double exponential smoothing
            convergence_check.set_parameters (slope_threshold, alpha, beta, buffer_len, min_iter);
            use_convergence_check = true;
            new_control_points_vec.resize(12);
          }

        private:
          bool use_convergence_check;
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> control_points;
          Eigen::Vector3d coherence_distance;
          Eigen::Matrix<default_type, 4, 1> stop_len, recip_spacing;
          DoubleExpSmoothSlopeCheck convergence_check;
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> new_control_points_vec;
      };

      class RigidRobustEstimator { MEMALIGN(RigidRobustEstimator)
        public:
          inline bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
              default_type step_size);
      };



      /** \addtogroup Transforms
      @{ */

      /*! A 3D rigid transformation class for registration.
       *
       */
      class Rigid : public Base  { MEMALIGN(Rigid)
        public:

          using ParameterType = typename Base::ParameterType;
          using UpdateType = RigidLinearNonSymmetricUpdate;
          using RobustEstimatorType = RigidRobustEstimator;
          using has_robust_estimator = int;

          Rigid () : Base (12) {
            default_type w1 (MR::File::Config::get_float ("RegGdWeightMatrix", 0.0003f));
            default_type w2 (MR::File::Config::get_float ("RegGdWeightTranslation", 1.0f));
            const Eigen::Vector4d weights (w1, w1, w1, w2);
            this->optimiser_weights << weights, weights, weights;
          }

          Eigen::Matrix<default_type, 4, 1> get_jacobian_vector_wrt_params (const Eigen::Vector3d& p) const ;

          Eigen::MatrixXd get_jacobian_wrt_params (const Eigen::Vector3d& p) const ;

          void set_parameter_vector (const Eigen::Matrix<ParameterType, Eigen::Dynamic, 1>& param_vector);

          void get_parameter_vector (Eigen::Matrix<ParameterType, Eigen::Dynamic, 1>& param_vector) const;

          UpdateType* get_gradient_descent_updator () {
            return &gradient_descent_updator;
          }

          bool robust_estimate (
            Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
            vector<Eigen::Matrix<default_type, Eigen::Dynamic, 1>>& grad_estimates,
            const Eigen::Matrix<default_type, 4, 4>& control_points,
            const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& parameter_vector,
            const default_type& weiszfeld_precision,
            const size_t& weiszfeld_iterations,
            default_type learning_rate) const;

        protected:
          UpdateType gradient_descent_updator;
          RobustEstimatorType robust_estimator;
      };
      //! @}
    }
  }
}

#endif
