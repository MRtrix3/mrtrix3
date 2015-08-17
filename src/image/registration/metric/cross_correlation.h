/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by Maximilian Pietsch, 05/02/2015

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

#ifndef __image_registration_metric_cross_correlation_h__
#define __image_registration_metric_cross_correlation_h__

#include "image/registration/metric/base.h"
#include "point.h"
#include "math/vector.h"
// #include <cmath>

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Metric
      {

        class CrossCorrelation : public Base {

          public:
            /** typedef int is_neighbourhood: type_trait to distinguish voxel-wise and neighbourhood based metric types */
            typedef int is_neighbourhood;

            template <class Params>
              double operator() (Params& params, const Image::Iterator& iter) { // , Math::Vector<double>& gradient)
                
                // params.transformation.get_jacobian_wrt_params (target_point, this->jacobian);
                // this->compute_moving_gradient (moving_point);
                // update gradient        

                return 0.0;
            }

          protected:
            

        };
      }
    }
  }
}
#endif
