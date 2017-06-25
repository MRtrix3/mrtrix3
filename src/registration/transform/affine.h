/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __registration_transform_affine_h__
#define __registration_transform_affine_h__

#include "registration/transform/base.h"
#include "types.h"
#include "file/config.h"
#include "math/math.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      enum TransformProjectionType {rigid_nonsym, affine, affine_nonsym, none};

      class AffineUpdate { MEMALIGN(AffineUpdate)
        public:
          AffineUpdate (): use_convergence_check (false), projection_type (TransformProjectionType::affine) { }

          bool operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
              default_type step_size);

          void set_control_points (
            const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& points,
            const Eigen::Vector3d& coherence_dist,
            const Eigen::Vector3d& stop_length,
            const Eigen::Vector3d& voxel_spacing );

          void set_projection_type (const TransformProjectionType& type) {
            INFO ("projection type set to: " + str(type));
            projection_type = type;
          }

          void set_convergence_check (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope_threshold,
                                      default_type alpha,
                                      default_type beta,
                                      size_t buffer_len,
                                      size_t min_iter) {
            convergence_check.set_parameters (slope_threshold, alpha, beta, buffer_len, min_iter);
            use_convergence_check = true;
            new_control_points_vec.resize(12);
          }

        private:
          bool use_convergence_check;
          TransformProjectionType projection_type;
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> control_points;
          Eigen::Vector3d coherence_distance;
          Eigen::Matrix<default_type, 4, 1> stop_len, recip_spacing;
          DoubleExpSmoothSlopeCheck convergence_check;
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> new_control_points_vec;
      };

      class AffineRobustEstimator { MEMALIGN(AffineRobustEstimator)
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
       */
      class Affine : public Base  { MEMALIGN(Affine)
        public:

          using ParameterType = typename Base::ParameterType;
          using UpdateType = AffineUpdate;
          using RobustEstimatorType = AffineRobustEstimator;
          using has_robust_estimator = int;


          Affine () : Base (12) {
            //CONF option: reg_gdweight_matrix
            //CONF default: 0.0003
            //CONF Linear registration: weight for optimisation of linear (3x3) matrix parameters
            default_type w1 (MR::File::Config::get_float ("reg_gdweight_matrix", 0.0003f));
            //CONF option: reg_gdweight_translation
            //CONF default: 1
            //CONF Linear registration: weight for optimisation of translation parameters
            default_type w2 (MR::File::Config::get_float ("reg_gdweight_translation", 1.0f));
            const Eigen::Vector4d weights (w1, w1, w1, w2);
            this->optimiser_weights << weights, weights, weights;
          }

          Eigen::Matrix<default_type, 4, 1> get_jacobian_vector_wrt_params (const Eigen::Vector3& p) const ;

          Eigen::MatrixXd get_jacobian_wrt_params (const Eigen::Vector3& p) const ;

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
