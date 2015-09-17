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

      template <class TransformType,
                class MovingImageType,
                class MovingImageInterpolatorType,
                class TemplateImageType,
                class MidwayImageType,
                class TemplateImageInterpolatorType,
                class MovingMaskInterpolatorType,
                class TemplateMaskInterpolatorType>
      class Params {
        public:

          typedef typename TransformType::ParameterType TransformParamType;
          typedef typename MovingImageInterpolatorType::value_type MovingValueType;
          typedef typename TemplateImageInterpolatorType::value_type TemplateValueType;

          Params (TransformType& transform,
                  MovingImageType& moving_image,
                  TemplateImageType& template_image,
                  MidwayImageType& midway_image) :
                    transformation (transform),
                    moving_image (moving_image),
                    template_image (template_image),
                    midway_image (midway_image),
                    sparsity(static_cast<double> (0.0)){
                      moving_image_interp.reset (new MovingImageInterpolatorType (moving_image));
                      template_image_interp.reset (new TemplateImageInterpolatorType (template_image));
          }

          void set_extent (std::vector<size_t> extent_vector) { extent=std::move(extent_vector); }

          const std::vector<size_t>& get_extent() const { return extent; }

          TransformType& transformation;
          MovingImageType moving_image;
          TemplateImageType template_image;
          MidwayImageType midway_image;
          MR::copy_ptr<MovingImageInterpolatorType> moving_image_interp;
          MR::copy_ptr<TemplateImageInterpolatorType> template_image_interp;
          MR::copy_ptr<MovingMaskInterpolatorType> moving_mask_interp;
          MR::copy_ptr<TemplateMaskInterpolatorType> template_mask_interp;
          double sparsity;
          std::vector<size_t> extent;
          MovingImageType im1_processed;
          TemplateImageType im2_processed;
          MR::copy_ptr<MovingImageInterpolatorType> im1_processed_interp;
          MR::copy_ptr<TemplateImageInterpolatorType> im2_processed_interp;
      };
    }
  }
}

#endif
