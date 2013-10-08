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

#ifndef __image_registration_metric_params_h__
#define __image_registration_metric_params_h__

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Metric
      {

        template <class TransformType,
                  class MovingImageVoxelType,
                  class MovingImageInterpolatorType,
                  class TemplateImageVoxelType,
                  class MovingMaskInterpolatorType,
                  class TemplateMaskInterpolatorType>
        class Params {
          public:

            typedef typename TransformType::ParameterType TransformParamType;

            Params (TransformType& transform,
                    MovingImageVoxelType& moving_image,
                    TemplateImageVoxelType& template_image) :
                      transformation (transform),
                      moving_image (moving_image),
                      template_image (template_image){
                        moving_image_interp = new MovingImageInterpolatorType (moving_image);
            }

            void set_moving_iterpolator (MovingImageVoxelType& moving_image) {
              moving_image_interp = new MovingImageInterpolatorType (moving_image);
            }

            TransformType& transformation;
            TemplateImageVoxelType moving_image;
            TemplateImageVoxelType template_image;
            Ptr<MovingImageInterpolatorType> moving_image_interp;
            Ptr<TemplateMaskInterpolatorType> template_mask_interp;
            Ptr<MovingMaskInterpolatorType> moving_mask_interp;
        };
      }
    }
  }
}

#endif
