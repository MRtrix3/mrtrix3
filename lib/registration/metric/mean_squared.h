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

#ifndef __registration_metric_mean_squared_h__
#define __registration_metric_mean_squared_h__

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      #ifdef NONSYMREGISTRATION
        class MeanSquared {

          public:
            template <class Params>
              default_type operator() (Params& params,
                                       const Eigen::Vector3 im2_point,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                if (isnan (default_type (params.im2_image.value())))
                  return 0.0;

                Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (im2_point);

                typename Params::Im1ValueType im1_value;
                typename Params::Im2ValueType im2_value;
                Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;
                Eigen::Matrix<typename Params::Im2ValueType, 1, 3> im2_grad;

                params.im1_image_interp->value_and_gradient (im1_value, im1_grad);
                params.im2_image_interp->value_and_gradient (im2_value, im2_grad);

                default_type diff = im1_value - params.im2_image.value();
                for (ssize_t par = 0; par < gradient.size(); par++) {
                  default_type sum = 0.0;
                  for ( size_t dim = 0; dim < 3; dim++)
                    sum += 2.0 * diff * jacobian (dim, par) * im1_grad[dim];
                  gradient[par] += sum;
                }
                return diff * diff;
            }

        };
      #else
        class MeanSquared {

          public:
            template <class Params>
              default_type operator() (Params& params,
                                       const Eigen::Vector3 im1_point,
                                       const Eigen::Vector3 im2_point,
                                       const Eigen::Vector3 midway_point,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (midway_point);

                typename Params::Im1ValueType im1_value;
                typename Params::Im2ValueType im2_value;
                Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;
                Eigen::Matrix<typename Params::Im2ValueType, 1, 3> im2_grad;

                params.im1_image_interp->value_and_gradient (im1_value, im1_grad);
                if (isnan (default_type (im1_value)))
                  return 0.0;
                params.im2_image_interp->value_and_gradient (im2_value, im2_grad);
                if (isnan (default_type (im2_value)))
                  return 0.0;

                default_type diff = im1_value - im2_value;
                for (ssize_t par = 0; par < gradient.size(); par++) {
                  default_type sum = 0.0;
                  for ( size_t dim = 0; dim < 3; dim++)
                    sum += diff * jacobian (dim, par) * (im1_grad[dim] + im2_grad[dim]);
                  gradient[par] += sum;
                }
                return diff * diff;
            }

        };
      #endif
    }
  }
}
#endif
