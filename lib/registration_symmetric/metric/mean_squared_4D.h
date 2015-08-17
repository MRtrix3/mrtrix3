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

#ifndef __image_registration_symmetric_metric_mean_squared_4D_h__
#define __image_registration_symmetric_metric_mean_squared_4D_h__

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
        class MeanSquared4D : public Base {
          public:

            template <class Params>
              double operator() (Params& params,
                                 const Point<double> target_point,
                                 const Point<double> moving_point,
                                 const Point<double> midspace_point,
                                 Math::Vector<double>& gradient) {

                assert (params.template_image.ndim() == 4);

                params.template_image[3] = 0;  // TODO
                if (isnan (double (params.template_image.value()))) 
                  return 0.0;
                params.transformation.get_jacobian_wrt_params (midspace_point, this->jacobian);

                double total_diff = 0.0;

                moving_gradient_interp->scanner (moving_point);

                for (params.template_image[3] = 0; params.template_image[3] < params.template_image.dim(3); ++params.template_image[3]) {
                  (*moving_gradient_interp)[4] = params.template_image[3];
                  (*params.moving_image_interp)[3] = params.template_image[3];

                  (*moving_gradient_interp)[3] = 0;
                  moving_grad[0] = moving_gradient_interp->value();
                  ++(*moving_gradient_interp)[3];
                  moving_grad[1] = moving_gradient_interp->value();
                  ++(*moving_gradient_interp)[3];
                  moving_grad[2] = moving_gradient_interp->value();

                  (*template_gradient_interp)[3] = 0;
                  template_grad[0] = template_gradient_interp->value();
                  ++(*template_gradient_interp)[3];
                  template_grad[1] = template_gradient_interp->value();
                  ++(*template_gradient_interp)[3];
                  template_grad[2] = template_gradient_interp->value();

                  double diff = params.moving_image_interp->value() - params.template_image_interp->value(); 
                  total_diff += (diff * diff);

                  throw Exception ("4D mean squared not implemented yet");
                  for (size_t par = 0; par < gradient.size(); par++) {
                    double sum = 0.0;
                    for ( size_t dim = 0; dim < 3; dim++)
                      // sum += 2.0 * diff * this->jacobian (dim, par) * moving_grad[dim];
                      sum += diff * this->jacobian (dim, par) * (moving_grad[dim] + template_grad[dim]); // symmetric
                    gradient[par] += sum;
                  }

                }
                return total_diff;
            }

        };
      }
    }
  }
}
#endif
