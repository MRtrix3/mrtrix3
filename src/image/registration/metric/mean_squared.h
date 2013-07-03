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

#include "image/registration/metric/base.h"
#include "point.h"
#include "math/vector.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Metric
      {
        class MeanSquared : public Base {
          public:

            template <class Params>
              double operator() (Params& params,
                                 const Point<double> target_point,
                                 const Point<double> moving_point,
                                 Math::Vector<double>& gradient) {

                params.transformation.get_jacobian_wrt_params (target_point, this->jacobian);

                if (params.template_image.ndim() == 4)
                  (*gradient_interp)[4] = params.template_image [3];

                this->compute_moving_gradient (moving_point);

                double diff = params.moving_image_interp->value() - params.template_image.value();

                for (size_t par = 0; par < gradient.size(); par++) {
                  double sum = 0.0;
                  for( size_t dim = 0; dim < 3; dim++) {
                    sum += 2.0 * diff * this->jacobian(dim, par) * moving_grad[dim];
                  }
                  gradient[par] += sum;
                }
                return diff * diff;
            }

        };
      }
    }
  }
}
#endif
