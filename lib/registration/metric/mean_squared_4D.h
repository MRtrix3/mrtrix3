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


namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class MeanSquared4D {
        public:
          #ifdef NONSYMREGISTRATION
          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3 im2_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              assert (params.im2_image.ndim() == 4);

              params.im2_image.index(3) = 0;
              if (isnan (default_type (params.im2_image.value())))
                return 0.0;

              Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (im2_point);

              default_type total_diff = 0.0;

              typename Params::Im1ValueType im1_value;
              Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;

              // TODO interpolate 4th dimension in one go
              for (params.im2_image.index(3) = 0; params.im2_image.index(3) < params.im2_image.size(3); ++params.im2_image.index(3)) {
                (*params.im1_image_interp).index(3) = params.im2_image.index(3);

                params.im1_image_interp->value_and_gradient (im1_value, im1_grad);
                default_type diff = im1_value - params.im2_image.value();
                total_diff += (diff * diff);

                for (ssize_t par = 0; par < gradient.size(); par++) {
                  default_type sum = 0.0;
                  for ( size_t dim = 0; dim < 3; dim++)
                    sum += 2.0 * diff * jacobian (dim, par) * im1_grad[dim];
                  gradient[par] += sum;
                }

              }
              return total_diff;
          }
          #else

          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3 im1_point,
                                     const Eigen::Vector3 im2_point,
                                     const Eigen::Vector3 midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
              Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (midway_point);

              Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 3> im1_grad;
              Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 3> im2_grad;
              Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 1> im1_value;
              Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 1> im2_value;


              params.im1_image_interp->value_and_gradient_row (im1_value, im1_grad);
              params.im2_image_interp->value_and_gradient_row (im2_value, im2_grad);
              for (size_t i = 0; i < params.im1_image.size(3); ++i) {
                if (isnan (default_type (im1_value[i])))
                  return 0.0;
                if (isnan (default_type (im2_value[i])))
                  return 0.0;
              }

              default_type total_diff = 0.0;
              for (size_t i = 0; i < params.im1_image.size(3); ++i) {

                default_type diff = im1_value[i] - im2_value[i];
                total_diff += diff * diff;
                for (ssize_t par = 0; par < gradient.size(); par++) {
                  default_type sum = 0.0;
                  for ( size_t dim = 0; dim < 3; dim++)
                    sum += diff * jacobian (dim, par) * (im1_grad(i, dim) + im2_grad(i, dim));
                  gradient[par] += sum / (default_type)params.im1_image.size(3);
                }
              }
              return total_diff / (default_type)params.im1_image.size(3);
          }
          #endif

      };
    }
  }
}
#endif
