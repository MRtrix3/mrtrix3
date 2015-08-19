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

#ifndef __image_registration_metric_mean_squared_h__
#define __image_registration_metric_mean_squared_h__

#include "registration/metric/base.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      class MeanSquared : public Base {

        public:
          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3 target_point,
                                     const Eigen::Vector3 moving_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
              if (isnan (default_type (params.template_image.value())))
                return 0.0;

              params.transformation.get_jacobian_wrt_params (target_point, this->jacobian);

              if (params.template_image.ndim() == 4) {
                (*gradient_interp).index(4) = params.template_image.index(4);
                (*params.moving_image_interp).index(3) = params.template_image.index(3);
              }

              this->compute_moving_gradient (moving_point);


//              CONSOLE(str(moving_point));

              default_type diff = params.moving_image_interp->value() - params.template_image.value();
              for (size_t par = 0; par < gradient.size(); par++) {
                default_type sum = 0.0;
                for ( size_t dim = 0; dim < 3; dim++)
                  sum += 2.0 * diff * this->jacobian (dim, par) * moving_grad[dim];
                gradient[par] += sum;
              }
              return diff * diff;
          }

      };
    }
  }
}
#endif
