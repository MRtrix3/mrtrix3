/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 16/11/2012

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

#ifndef __registration_nonlinear_h__
#define __registration_nonlinear_h__

#include "ptr.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "image/filter/resize.h"
#include "image/interp/linear.h"
#include "image/interp/nearest.h"
#include "math/matrix.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {

      class NonLinear
      {

        public:

          typedef float value_type;

          NonLinear () :
            max_iter (1, 300),
            scale_factor (2),
            gradient_smooth_stdev (2, 3.0),
            deformation_smooth_stdev (2, 0.0) {
            scale_factor[0] = 0.5;
            scale_factor[1] = 1;
          }

          NonLinear (const std::vector<int>& max_iter,
               const std::vector<value_type>& scale_factor,
               const std::vector<value_type>& gradient_smooth_stdev,
               const std::vector<value_type>& deformation_smooth_stdev ) :
               max_iter (max_iter),
               scale_factor (scale_factor),
               gradient_smooth_stdev (gradient_smooth_stdev),
               deformation_smooth_stdev (deformation_smooth_stdev) { }

          void set_max_iter (const std::vector<int>& val) {
            max_iter = val;
          }

          void set_scale_factor (const std::vector<value_type>& val) {
            scale_factor = val;
          }

          void set_gradient_smooth_stdev (const std::vector<value_type>& val) {
            gradient_smooth_stdev = val;
          }

          void set_deformation_smooth_stdev (const std::vector<value_type>& val) {
            deformation_smooth_stdev = val;
          }

          template <class MetricType, class MovingImageType, class TemplateImageType>
          void run (
            MetricType& metric,
            MovingImageType& moving_image,
            TemplateImageType& template_image) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, MovingImageType, TemplateImageType, BogusMaskType, BogusMaskType >
                (metric, moving_image, template_image, NULL, NULL);
            }

          template <class MetricType, class MovingImageType, class TemplateImageType, class TemplateMaskType>
          void run_template_mask (
            MetricType& metric,
            MovingImageType& moving_image,
            TemplateImageType& template_image,
            Ptr<TemplateMaskType>& template_mask) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, MovingImageType, TemplateImageType, BogusMaskType, TemplateMaskType >
                (metric, moving_image, template_image, NULL, template_mask);
            }


          template <class MetricType, class MovingImageType, class TemplateImageType, class MovingMaskType>
          void run_moving_mask (
            MetricType& metric,
            MovingImageType& moving_image,
            TemplateImageType& template_image,
            Ptr<MovingMaskType>& moving_mask) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, MovingImageType, TemplateImageType, MovingMaskType, BogusMaskType >
                (metric, moving_image, template_image, moving_mask, NULL);
            }

          template <class MetricType, class MovingImageType, class TemplateImageType, class MovingMaskType, class TemplateMaskType>
          void run_masked (
            MetricType& metric,
            MovingImageType& moving_image,
            TemplateImageType& template_image,
            Ptr<MovingMaskType>& moving_mask,
            Ptr<TemplateMaskType>& template_mask) {

              typedef typename MovingMaskType::voxel_type MovingMaskVoxelType;
              typedef typename TemplateMaskType::voxel_type TemplateMaskVoxelType;
              MovingMaskVoxelType moving_image_vox (moving_image);
              TemplateMaskVoxelType template_image_vox (template_image);

              if (max_iter.size() == 1)
                max_iter.resize (scale_factor.size(), max_iter[0]);
              else if (max_iter.size() != scale_factor.size())
                throw Exception ("the max number of iterations needs to be defined for each multi-resolution level");
              for (size_t level = 0; level < scale_factor.size(); ++level) {
                if (scale_factor[level] <= 0 || scale_factor[level] > 1)
                  throw Exception ("the scale factor for each multi-resolution level must be between 0 and 1");
              }

              typedef Image::Interp::Linear<Image::BufferScratch<value_type>::voxel_type > MovingImageInterpolatorType;

              for (size_t level = 0; level < scale_factor.size(); level++) {

                CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

                Image::Filter::Resize moving_resize_filter (moving_image_vox);
                Image::Filter::Resize template_resize_filter (template_image_vox);
                moving_resize_filter.set_scale_factor (scale_factor[level]);
                moving_resize_filter.set_interp_type (1);
                Image::BufferScratch<value_type> moving_resized (moving_resize_filter.info());
                Image::BufferScratch<value_type>::voxel_type moving_resized_vox (moving_resized);
                Image::Filter::GaussianSmooth<float> moving_smooth_filter (moving_resized_vox);
                Image::BufferScratch<value_type> moving_resized_smoothed (moving_smooth_filter.info());
                Image::BufferScratch<value_type>::voxel_type moving_resized_smoothed_vox (moving_resized_smoothed);

                template_resize_filter.set_scale_factor (scale_factor[level]);
                template_resize_filter.set_interp_type (1);
                Image::BufferScratch<value_type> template_buffer (template_resize_filter.info());
                Image::BufferScratch<value_type>::voxel_type template_vox (template_buffer);
                {
                  LogLevelLatch log_level (0);
                  moving_resize_filter (moving_image_vox, moving_resized_vox);
                  moving_smooth_filter (moving_resized_vox, moving_resized_smoothed_vox);
                  template_resize_filter (template_image_vox, template_vox);
                }
                MovingImageInterpolatorType moving_interp (moving_resized_smoothed_vox);
                metric.set_moving_image (moving_resized_smoothed_vox);
//                ParamType parameters (transform, moving_interp, template_vox);
//
//                Ptr<MovingMaskVoxelType> moving_mask_vox;
//                Ptr<TemplateMaskVoxelType> template_mask_vox;
//                if (moving_mask) {
//                  moving_mask_vox = new MovingMaskVoxelType (*moving_mask);
//                  parameters.moving_mask_interp = new Image::Interp::Nearest<MovingMaskVoxelType> (*moving_mask_vox);
//                }
//                if (template_mask) {
//                  template_mask_vox = new TemplateMaskVoxelType (*template_mask);
//                  parameters.template_mask_interp = new Image::Interp::Nearest<TemplateMaskVoxelType> (*template_mask_vox);
//                }

                /**
                 *  Allocate displacement fields
                 *
                 *
                 */





              }
            }

        protected:
          std::vector<int> max_iter;
          std::vector<value_type> scale_factor;
          std::vector<value_type> gradient_smooth_stdev;
          std::vector<value_type> deformation_smooth_stdev;

      };
    }
  }
}

#endif
