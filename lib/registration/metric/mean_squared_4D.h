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


#ifndef __registration_metric_mean_squared_4D_h__
#define __registration_metric_mean_squared_4D_h__

#include "math/math.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <class Im1Type, class Im2Type>
      class MeanSquared4D {
        public:
          MeanSquared4D (Im1Type im1, Im2Type im2) {} //TODO

          template <class Params>
          default_type operator() (Params& params,
                                   const Eigen::Vector3 im1_point,
                                   const Eigen::Vector3 im2_point,
                                   const Eigen::Vector3 midway_point,
                                   Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

            ssize_t volumes = params.im1_image_interp->size(3);

            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 3> im1_grad (volumes, 3);
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 3> im2_grad (volumes, 3);
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> im1_values (volumes);
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> diff_values (volumes);
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 1> im2_values (volumes);


            params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
            if (im1_values.hasNaN())
              return 0.0;

            params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);
            if (im2_values.hasNaN())
              return 0.0;

            const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);
            diff_values = im1_values - im2_values;

            for (ssize_t i = 0; i < volumes; ++i) {
              const Eigen::Vector3d g = diff_values[i] * (im1_grad.row(i) + im2_grad.row(i));
              gradient.segment<4>(0) += g(0) * jacobian_vec;
              gradient.segment<4>(4) += g(1) * jacobian_vec;
              gradient.segment<4>(8) += g(2) * jacobian_vec;
            }

            return diff_values.squaredNorm() / (default_type)volumes;
        }

        private:
//          Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 3> im1_grad;
//          Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 3> im2_grad;
//          Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> im1_values, diff_values;
//          Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 1> im2_values;
      };
    }
  }
}
#endif
