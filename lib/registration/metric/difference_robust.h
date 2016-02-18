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

#ifndef __registration_metric_difference_robust_h__
#define __registration_metric_difference_robust_h__

#include "registration/metric/m_estimators.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template<class Estimator = L2>
        class DifferenceRobust {
          public:
            DifferenceRobust(Estimator est) : estimator(est) {}

            template <class Params>
              default_type operator() (Params& params,
                                       const Eigen::Vector3 im1_point,
                                       const Eigen::Vector3 im2_point,
                                       const Eigen::Vector3 midway_point,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

                typename Params::Im1ValueType im1_value;
                typename Params::Im2ValueType im2_value;
                Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;
                Eigen::Matrix<typename Params::Im2ValueType, 1, 3> im2_grad;

                params.im1_image_interp->value_and_gradient_wrt_scanner (im1_value, im1_grad);
                if (isnan (default_type (im1_value)))
                  return 0.0;
                params.im2_image_interp->value_and_gradient_wrt_scanner (im2_value, im2_grad);
                if (isnan (default_type (im2_value)))
                  return 0.0;

                default_type residual, grad;
                estimator((default_type) im1_value - (default_type) im2_value, residual, grad);
                const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);
                const  Eigen::Vector3d g = grad * (im1_grad + im2_grad);
                gradient.segment<4>(0) += g(0) * jacobian_vec;
                gradient.segment<4>(4) += g(1) * jacobian_vec;
                gradient.segment<4>(8) += g(2) * jacobian_vec;

                return residual;
            }

            Estimator estimator;
        };
    }
  }
}
#endif
