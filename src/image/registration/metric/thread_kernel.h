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

#ifndef __image_registration_metric_threadkernel_h__
#define __image_registration_metric_threadkernel_h__

#include "math/matrix.h"
#include "image/voxel.h"
#include "image/iterator.h"
#include "image/transform.h"
#include "point.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Metric
      {
        template <class MetricType, class ParamType>
        class ThreadKernel {
          public:
            ThreadKernel (const MetricType& metric, ParamType& parameters, double& overall_cost_function, Math::Vector<double>& overall_gradient) :
              metric (metric),
              params (parameters),
              cost_function (0.0),
              gradient (overall_gradient.size()),
              overall_cost_function (overall_cost_function),
              overall_gradient (overall_gradient),
              transform (params.template_image) {
                gradient.zero();
            }

            ~ThreadKernel () {
              overall_cost_function += cost_function;
              overall_gradient += gradient;
            }

            void operator() (const Image::Iterator& iter) {

              Point<float> template_point = transform.voxel2scanner (iter);
              if (params.template_mask_interp) {
                params.template_mask_interp->scanner (template_point);
                if (!params.template_mask_interp->value())
                  return;
              }

              Point<float> moving_point;
              Math::Vector<double> param;
              params.transformation.get_parameter_vector (param);
              params.transformation.transform (moving_point, template_point);
              if (params.moving_mask_interp) {
                params.moving_mask_interp->scanner (moving_point);
                if (!params.moving_mask_interp->value())
                  return;
              }
              Image::voxel_assign (params.template_image, iter);
              params.moving_image_interp->scanner (moving_point);
              if (!(*params.moving_image_interp))
                return;
              cost_function += metric (params, template_point, moving_point, gradient);
            }

            protected:
              MetricType metric;
              ParamType params;

              double cost_function;
              Math::Vector<double> gradient;
              double& overall_cost_function;
              Math::Vector<double>& overall_gradient;
              Image::Transform transform;
        };
      }
    }
  }
}

#endif
