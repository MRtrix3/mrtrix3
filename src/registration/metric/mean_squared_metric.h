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

#ifndef __mean_squared_metric_h__
#define __mean_squared_metric_h__

#include "image/iterator.h"
#include "image/voxel.h"

namespace MR {
  namespace Registration {
    namespace Metric {

      class MeanSquaredMetric {
        public:
          MeanSquaredMetric () {}

          template <class ImageVoxelType, class MaskVoxelType, class TransformType, class InterpolatorType>
          class BaseParams {
            public:

              BaseParams(ImageVoxelType& fixed_image,
                         ImageVoxelType& moving_image,
                         TransformType& transform,
                         InterpolatorType& interpolator) :
                         fixed_image_(fixed_image),
                         moving_image_(moving_image),
                         transform_(transform),
                         interpolator_(interpolator){}


              void set_fixed_mask(MaskVoxelType* fixed_mask) {
                fixed_mask_ = fixed_mask;
              }

              void set_moving_mask(MaskVoxelType* moving_mask) {
                moving_mask_ = moving_mask;
              }

              ImageVoxelType fixed_image_;
              ImageVoxelType moving_image_;
              TransformType transform_;
              InterpolatorType interpolator_;
              Ptr<MaskVoxelType> fixed_mask_;
              Ptr<MaskVoxelType> moving_mask_;

          };

        class Kernel  {
          public:
            Kernel (const BaseParams& params, Math::Vector<float>& overall_gradient, double& overall_cost_function) :
              params_(params),
              ref_overall_cost_function (overall_cost_function),
              ref_overall_gradient (overall_gradient) {}
            ~Kernel () {
              ref_overall_cost_function += cost_function;
              ref_overall_gradient += gradient;
            }


            void operator() (const Image::Iterator& pos)
            {
              voxel_assign((*fixed), pos);
              cost_function += 1.0;
            }

            BaseParams& params_;

            double cost_function;
            Math::Vector<float> gradient;
            double& ref_overall_cost_function;
            Math::Vector<float> ref_overall_gradient;
        };

        float operator() (Math::Vector<float>& params, Math::Vector<float>& gradient) {
          overall_cost_function_ = 0.0;
          over_all_gradient_.zero();


          Kernel kernel (source, destination);
          threaded_loop (copy_kernel, source, axes, num_axes_in_thread);

           gradient = over_all_gradient_;
           return overall_cost_function_;
        }


        double overall_cost_function_;
        Math::Vector<float> over_all_gradient_;


    };
  }

  }
}

#endif
