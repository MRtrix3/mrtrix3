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
                  MidwayImageType& midway_image) :
                    transformation (transform),
                    im1_image (im1_image),
                    im2_image (im2_image),
                    midway_image (midway_image),
                    sparsity(0.0){
                      im1_image_interp.reset (new Im1ImageInterpolatorType (im1_image));
                      im2_image_interp.reset (new Im2ImageInterpolatorType (im2_image));
          }

          void set_extent (std::vector<size_t> extent_vector) { extent=std::move(extent_vector); }

          const std::vector<size_t>& get_extent() const { return extent; }

          TransformType& transformation;
          Im1ImageType im1_image;
          Im2ImageType im2_image;
          MidwayImageType midway_image;
          MR::copy_ptr<Im1ImageInterpolatorType> im1_image_interp;
          MR::copy_ptr<Im2ImageInterpolatorType> im2_image_interp;
          default_type sparsity;
          std::vector<size_t> extent;
          Image<bool> im1_mask;
          Image<bool> im2_mask;
          MR::copy_ptr<Im1MaskInterpolatorType> im1_mask_interp;
          MR::copy_ptr<Im2MaskInterpolatorType> im2_mask_interp;

          ProcessedImageType processed_image;
          MR::copy_ptr<ProcessedImageInterpolatorType> processed_image_interp;
          ProcessedMaskType processed_mask;
          MR::copy_ptr<ProcessedMaskInterpolatorType> processed_mask_interp;
      };
    }
  }
}

#endif
