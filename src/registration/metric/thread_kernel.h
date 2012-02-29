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

#include "image/voxel.h"
#include "image/iterator.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <class MetricType, class ParamType>
      class ThreadKernel {
        public:
          ThreadKernel (const MetricType& metric, ParamType& parameters, double& overall_cost_function, Math::Vector<float>& overall_gradient) :
            metric_ (metric),
            param_ (parameters),
            overall_cost_function_ (overall_cost_function),
            overall_gradient_ (overall_gradient) {
            std::cout << "asdf2" << std::endl;
          }

          ~ThreadKernel () {
            overall_cost_function_ += cost_function_;
            overall_gradient_ += gradient_;
          }

          void operator() (const Image::Iterator& iter) {

            std::cout << "voxel " << iter[0] << " " << iter[1] << " " <<  iter[2] << std::endl;

//            if (param_.target_mask) {
//                Image::voxel_assign (*param_.target_mask, iter);
//                if (!param_.target_mask->value())
//                return;
//            }
//            Math::Vector<float> point(3);
//            Math::mult(point,param_.target_image.transform(), iter);
//            std::cout << point << std::endl;

//            Image::voxel_assign (*param_.fixed_image, iter);
            // .... //
//            cost_function_ += metric_ (param_, gradient_);
          }

          protected:
            MetricType metric_;
            ParamType param_;

            double cost_function_;
            Math::Vector<float> gradient_;
            double overall_cost_function_;
            Math::Vector<float> overall_gradient_;
      };
    }
  }
}

#endif
