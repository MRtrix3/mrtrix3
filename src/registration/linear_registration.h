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

#ifndef __registration_linear_registration_h__
#define __registration_linear_registration_h__

#include "ptr.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "image/filter/resize.h"
#include "image/interp/linear.h"
#include "image/interp/nearest.h"
#include "registration/metric/params.h"
#include "registration/metric/evaluate.h"
#include "registration/transform/initialiser.h"
#include "math/matrix.h"
#include "math/gradient_descent.h"

namespace MR
{
  namespace Registration
  {

    class LinearRegistration
    {

      public:

        LinearRegistration () :
          max_iter_ (1, 300),
          scale_factor_ (2) {
          scale_factor_[0] = 0.5;
          scale_factor_[1] = 1;
        }


        LinearRegistration (const std::vector<int>& max_iter,
                            const std::vector<float>& resolution) :
            max_iter_ (max_iter),
            scale_factor_ (resolution),
            init_type_ (Transform::Init::mass) { }


        void set_max_iter (const std::vector<int>& max_iter) {
          max_iter_ = max_iter;
        }


        void set_scale_factor (const std::vector<float>& scale_factor) {
          scale_factor_ = scale_factor;
        }


        void set_init_type (Transform::Init::InitType type) {
          init_type_ = type;
        }


        template <class MetricType, class TransformType, class MovingImageVoxelType, class TargetImageVoxelType>
        void run (
          MetricType& metric,
          TransformType& transform,
          MovingImageVoxelType& moving_image,
          TargetImageVoxelType& target_image) {
            typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageVoxelType, TargetImageVoxelType, BogusMaskType, BogusMaskType >
              (metric, transform, moving_image, target_image, NULL, NULL);
          }


        template <class MetricType, class TransformType, class MovingImageVoxelType, class TargetImageVoxelType, class TargetMaskVoxelType>
        void run_target_mask (
          MetricType& metric,
          TransformType& transform,
          MovingImageVoxelType& moving_image,
          TargetImageVoxelType& target_image,
          Ptr<TargetMaskVoxelType>& target_mask) {
            typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageVoxelType, TargetImageVoxelType, BogusMaskType, TargetMaskVoxelType >
              (metric, transform, moving_image, target_image, NULL, target_mask);
          }


        template <class MetricType, class TransformType, class MovingImageVoxelType, class TargetImageVoxelType, class MovingMaskVoxelType>
        void run_moving_mask (
          MetricType& metric,
          TransformType& transform,
          MovingImageVoxelType& moving_image,
          TargetImageVoxelType& target_image,
          Ptr<MovingMaskVoxelType>& moving_mask) {
            typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageVoxelType, TargetImageVoxelType, MovingMaskVoxelType, BogusMaskType >
              (metric, transform, moving_image, target_image, moving_mask, NULL);
          }


        template <class MetricType, class TransformType, class MovingImageVoxelType, class TargetImageVoxelType, class MovingMaskVoxelType, class TargetMaskVoxelType>
        void run_masked (
          MetricType& metric,
          TransformType& transform,
          MovingImageVoxelType& moving_image,
          TargetImageVoxelType& target_image,
          Ptr<MovingMaskVoxelType>&  moving_mask,
          Ptr<TargetMaskVoxelType>& target_mask) {
            if (max_iter_.size() == 1)
              max_iter_.resize (scale_factor_.size(), max_iter_[0]);
            else if (max_iter_.size() != scale_factor_.size())
              throw Exception ("the max number of iterations needs to be defined for each multi-resolution level");
            for (size_t level = 0; level < scale_factor_.size(); ++level) {
              if (scale_factor_[level] <= 0 || scale_factor_[level] > 1)
                throw Exception ("the scale factor for each multi-resolution level must be between 0 and 1");
            }

            if (init_type_ == Transform::Init::mass)
              Transform::Init::initialise_using_image_mass (moving_image, target_image, transform);
            else if (init_type_ == Transform::Init::centre)
              Transform::Init::initialise_using_image_centres (moving_image, target_image, transform);

            typedef Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > MovingImageInterpolatorType;

            typedef Metric::Params<TransformType,
                                    MovingImageInterpolatorType,
                                    Image::BufferScratch<float>::voxel_type,
                                    Image::Interp::Nearest<MovingMaskVoxelType >,
                                    Image::Interp::Nearest<TargetMaskVoxelType > > ParamType;

            for (size_t level = 0; level < scale_factor_.size(); level++) {
              CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor_[level]));

              Image::Filter::Resize target_resize_filter (target_image);
              target_resize_filter.set_interp_type(1);
              target_resize_filter.set_scale_factor (scale_factor_[level]);
              Image::BufferScratch<float> target (target_resize_filter.info());
              Image::BufferScratch<float>::voxel_type target_vox (target);
              Image::BufferScratch<float> target_temp (target_resize_filter.info());
              Image::BufferScratch<float>::voxel_type target_temp_vox (target_temp);
              Image::Filter::GaussianSmooth target_smooth_filter (target_temp_vox);
              Image::BufferScratch<float> target_smoothed (target_smooth_filter.info());
              Image::BufferScratch<float>::voxel_type target_smoothed_vox (target_smoothed);

              Image::Filter::Resize moving_resize_filter (moving_image);
              moving_resize_filter.set_interp_type(1);
              moving_resize_filter.set_scale_factor (scale_factor_[level]);
              Image::BufferScratch<float> temp (moving_resize_filter.info());
              Image::BufferScratch<float>::voxel_type moving_temp_vox (temp);
              Image::Filter::GaussianSmooth moving_smooth_filter (moving_temp_vox);
              Image::BufferScratch<float> moving_smoothed (moving_smooth_filter.info());
              Image::BufferScratch<float>::voxel_type moving_smoothed_vox (moving_smoothed);
              {
                LogLevelLatch log_level (0);
                target_resize_filter (target_image, target_temp_vox);
                target_smooth_filter (target_temp_vox, target_smoothed_vox);
                moving_resize_filter (moving_image, moving_temp_vox);
                moving_smooth_filter (moving_temp_vox, moving_smoothed_vox);
              }

              MovingImageInterpolatorType moving_interp (moving_smoothed_vox);
              metric.set_moving_image (moving_smoothed_vox);

              ParamType parameters (transform, moving_interp, target_smoothed_vox);

              if (target_mask)
                parameters.target_mask_interp = new Image::Interp::Nearest<TargetMaskVoxelType> (*target_mask);
              if (moving_mask)
                parameters.moving_mask_interp = new Image::Interp::Nearest<MovingMaskVoxelType> (*moving_mask);

              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              Math::GradientDescent<Metric::Evaluate<MetricType, ParamType> > optim (evaluate);

              optim.run (max_iter_[level]);
              parameters.transformation.set_parameter_vector (optim.state());
            }
          }

      protected:
        std::vector<int> max_iter_;
        std::vector<float> scale_factor_;
        Transform::Init::InitType init_type_;
    };

  }
}

#endif
