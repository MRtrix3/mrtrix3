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

#ifndef __registration_metric_params_h__
#define __registration_metric_params_h__

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class TransformType, class MovingImageInterpolatorType, class TargetImageVoxelType, class MovingMaskInterpolatorType, class TargetMaskInterpolatorType>
      class Params {
        public:

          typedef typename TransformType::ParameterType TransformParamType;

          Params (TransformType& transform,
                  MovingImageInterpolatorType& moving_image,
                  TargetImageVoxelType& target_image) :
                    transformation (transform),
                    moving_image_interp (moving_image),
                    target_image (target_image){ }

          TransformType& transformation;
          MovingImageInterpolatorType moving_image_interp;
          TargetImageVoxelType target_image;
          Ptr<TargetMaskInterpolatorType> target_mask_interp;
          Ptr<MovingMaskInterpolatorType> moving_mask_interp;
      };
    }
  }
}

#endif
