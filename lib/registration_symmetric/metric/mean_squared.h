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

#ifndef __image_registration_symmetric_metric_mean_squared_h__
#define __image_registration_symmetric_metric_mean_squared_h__

#include "image/registration_symmetric/metric/base.h"
#include "point.h"
#include "math/vector.h"

namespace MR
{
  namespace Image
  {
    namespace RegistrationSymmetric
    {
      namespace Metric
      {

        class MeanSquared : public Base {

          public:
            template <class Params>
              double operator() (Params& params,
                                 const Point<double> template_point,
                                 const Point<double> moving_point,
                                 const Point<double> midspace_point,
                                 Math::Vector<double>& gradient) {
                params.transformation.get_jacobian_wrt_params (midspace_point, this->jacobian);

                this->compute_moving_gradient (moving_point);
                this->compute_template_gradient (template_point);

                double diff = params.moving_image_interp->value() - params.template_image_interp->value();
                for (size_t par = 0; par < gradient.size(); par++) {
                  double sum = 0.0;
                  for ( size_t dim = 0; dim < 3; dim++)
                    // sum += 2 * diff * this->jacobian (dim, par) * (moving_grad[dim] ); // original TODO why 2 * diff?
                    sum += 0.5 * diff * this->jacobian (dim, par) * (moving_grad[dim] + template_grad[dim]); // symmetric
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
