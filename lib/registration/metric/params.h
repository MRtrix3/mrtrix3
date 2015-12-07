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
                class Im1ImageType,
                class Im2ImageType,
                class MidwayImageType,
                class Im1MaskType,
                class Im2MaskType,
                class Im1ImageInterpolatorType,
                class Im2ImageInterpolatorType,
                class Im1MaskInterpolatorType,
                class Im2MaskInterpolatorType,
                class ProcessedImageType,
                class ProcessedImageInterpolatorType,
                class ProcessedMaskType,
                class ProcessedMaskInterpolatorType>
      class Params {
        public:

          typedef typename TransformType::ParameterType TransformParamType;
          typedef typename Im1ImageInterpolatorType::value_type Im1ValueType;
          typedef typename Im2ImageInterpolatorType::value_type Im2ValueType;
          typedef typename ProcessedImageInterpolatorType::value_type ImProcessedValueType;
          typedef ProcessedMaskType ImProcessedMaskType;
          typedef ProcessedMaskInterpolatorType ImProcessedMaskInterpolatorType;

          Params (TransformType& transform,
                  Im1ImageType& im1_image,
                  Im2ImageType& im2_image,
                  MidwayImageType& midway_image,
                  Im1MaskType& im1_mask,
                  Im2MaskType& im2_mask) :
                    transformation (transform),
                    im1_image (im1_image),
                    im2_image (im2_image),
                    midway_image (midway_image),
                    im1_mask (im1_mask),
                    im2_mask (im2_mask),
                    loop_density(1.0),
                    robust_estimate(false) {
                      im1_image_interp.reset (new Im1ImageInterpolatorType (im1_image));
                      im2_image_interp.reset (new Im2ImageInterpolatorType (im2_image));
                      if (im1_mask.valid())
                        im1_mask_interp.reset (new Im1MaskInterpolatorType (im1_mask));
                      if (im2_mask.valid())
                        im2_mask_interp.reset (new Im1MaskInterpolatorType (im2_mask));
          }

          void set_extent (std::vector<size_t> extent_vector) { extent=std::move(extent_vector); }
          
          void update_interpolators (){
            im1_image_interp.reset (new Im1ImageInterpolatorType (im1_image));
            im2_image_interp.reset (new Im2ImageInterpolatorType (im2_image));
          }


          const std::vector<size_t>& get_extent() const { return extent; }

          TransformType& transformation;
          Im1ImageType im1_image;
          Im2ImageType im2_image;
          MidwayImageType midway_image;
          MR::copy_ptr<Im1ImageInterpolatorType> im1_image_interp;
          MR::copy_ptr<Im2ImageInterpolatorType> im2_image_interp;
          Im1MaskType im1_mask;
          Im2MaskType im2_mask;
          MR::copy_ptr<Im1MaskInterpolatorType> im1_mask_interp;
          MR::copy_ptr<Im2MaskInterpolatorType> im2_mask_interp;
          default_type loop_density;
          bool robust_estimate;
          std::vector<size_t> extent;

          ProcessedImageType processed_image;
          MR::copy_ptr<ProcessedImageInterpolatorType> processed_image_interp;
          ProcessedMaskType processed_mask;
          MR::copy_ptr<ProcessedMaskInterpolatorType> processed_mask_interp;
      };
    }
  }
}

#endif
