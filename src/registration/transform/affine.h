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
#include "file/config.h"
#include "math/math.h"

using namespace MR::Math;

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      class AffineLinearNonSymmetricUpdate { MEMALIGN(AffineLinearNonSymmetricUpdate)
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

      class AffineUpdate { MEMALIGN(AffineUpdate)
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

          typedef typename Base::ParameterType ParameterType;
          typedef AffineUpdate UpdateType;
          typedef AffineRobustEstimator RobustEstimatorType;
          typedef int has_robust_estimator;


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
