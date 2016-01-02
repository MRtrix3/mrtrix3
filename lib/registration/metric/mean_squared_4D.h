/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 10/07/13

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

#ifndef __registration_metric_mean_squared_4D_h__
#define __registration_metric_mean_squared_4D_h__

#include "math/math.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class MeanSquared4D {
        public:

        template <class Params>
          default_type operator() (Params& params,
                                   const Eigen::Vector3 im1_point,
                                   const Eigen::Vector3 im2_point,
                                   const Eigen::Vector3 midway_point,
                                   Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

            Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 3> im1_grad, im2_grad;
            Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 1> im1_values, im2_values, diff_values;

            params.im1_image_interp->value_and_gradient_row (im1_values, im1_grad);
            if (im1_values.hasNaN())
              return 0.0;

            const ssize_t volumes = im1_values.size();
            assert (volumes > 1);

            params.im2_image_interp->value_and_gradient_row (im2_values, im2_grad);
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
      };
    }
  }
}
#endif
