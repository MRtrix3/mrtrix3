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

#ifndef __registration_metric_threadkernel_h__
#define __registration_metric_threadkernel_h__

#include "math/matrix.h"
#include "image/voxel.h"
#include "image/iterator.h"
#include "image/transform.h"
#include "point.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <class MetricType, class ParamType>
      class ThreadKernel {
        public:
          ThreadKernel (const MetricType& metric, ParamType& parameters, double& overall_cost_function, Math::Vector<double>& overall_gradient) :
            metric_ (metric),
            params_ (parameters),
            cost_function_ (0.0),
            gradient_ (overall_gradient.size()),
            overall_cost_function_ (overall_cost_function),
            overall_gradient_ (overall_gradient),
            transform_ (params_.target_image) {
              gradient_.zero();
          }

          ~ThreadKernel () {
            overall_cost_function_ += cost_function_;
            overall_gradient_ += gradient_;
          }

          void operator() (const Image::Iterator& iter) {

            Point<float> target_point = transform_.voxel2scanner (iter);
            if (params_.target_mask_interp) {
              params_.target_mask_interp->scanner (target_point);
              if (!params_.target_mask_interp->value())
                return;
            }

            Point<float> moving_point;
            params_.transformation.transform (moving_point, target_point);

            if (params_.moving_mask_interp) {
              params_.moving_mask_interp->scanner (moving_point);
              if (!params_.moving_mask_interp->value())
                return;
            }
            Image::voxel_assign (params_.target_image, iter);
            params_.moving_image_interp.scanner (moving_point);
            if (!params_.moving_image_interp)
              return;
            cost_function_ += metric_ (params_, target_point, moving_point, gradient_);
          }

          protected:

            MetricType metric_;
            ParamType params_;

            double cost_function_;
            Math::Vector<double> gradient_;
            double& overall_cost_function_;
            Math::Vector<double>& overall_gradient_;
            Image::Transform transform_;
      };
    }
  }
}

#endif
