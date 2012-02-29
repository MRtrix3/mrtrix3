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
#include "math/matrix.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "registration/metric/params.h"
#include "registration/metric/evaluate.h"
#include "math/gradient_descent.h"

namespace MR
{
  namespace Registration
  {
    class LinearRegistration
    {

      public:
        LinearRegistration () :
            max_iter_ (1, 300) { }


        LinearRegistration (const std::vector<int>& max_iter) :
            max_iter_ (max_iter) { }


        void set_max_iter (const std::vector<int>& max_iter) {
          max_iter_ = max_iter;
        }


        template <class MetricType, class TransformType, class MovingImageInterpolatorType, class TargetImageVoxelType>
          void run (
              MetricType& metric,
              TransformType& transform,
              MovingImageInterpolatorType& moving_image,
              TargetImageVoxelType& target_image) {
            typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageInterpolatorType, TargetImageVoxelType, BogusMaskType, BogusMaskType >
              (metric, transform, moving_image, target_image, NULL, NULL);
          }


        template <class MetricType, class TransformType, class MovingImageInterpolatorType, class TargetImageVoxelType, class TargetMaskVoxelType>
          void run_target_mask (
              MetricType& metric,
              TransformType& transform,
              MovingImageInterpolatorType& moving_image,
              TargetImageVoxelType& target_image,
              TargetMaskVoxelType* target_mask) {
            typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageInterpolatorType, TargetImageVoxelType, BogusMaskType, TargetMaskVoxelType >
              (metric, transform, moving_image, target_image, NULL, target_mask);
          }


        template <class MetricType, class TransformType, class MovingImageInterpolatorType, class TargetImageVoxelType, class MovingMaskInterpolatorType>
          void run_moving_mask (
              MetricType& metric,
              TransformType& transform,
              MovingImageInterpolatorType& moving_image,
              TargetImageVoxelType& target_image,
              MovingMaskInterpolatorType* moving_mask) {
            typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageInterpolatorType, TargetImageVoxelType, MovingMaskInterpolatorType, BogusMaskType >
              (metric, transform, moving_image, target_image, moving_mask, NULL);
          }


        template <class MetricType, class TransformType, class MovingImageInterpolatorType, class TargetImageVoxelType, class MovingMaskInterpolatorType, class TargetMaskVoxelType>
          void run_masked (
              MetricType& metric,
              TransformType& transform,
              MovingImageInterpolatorType& moving_image,
              TargetImageVoxelType& target_image,
              MovingMaskInterpolatorType* moving_mask,
              TargetMaskVoxelType* target_mask) {

            for (size_t iter = 0; iter < max_iter_.size(); iter++) {

              typedef Metric::Params<TransformType, MovingImageInterpolatorType, TargetImageVoxelType, MovingMaskInterpolatorType, TargetMaskVoxelType> ParamType;
              ParamType parameters (transform, moving_image, target_image);
              if (target_mask) parameters.target_mask = new TargetMaskVoxelType (*target_mask);
              if (moving_mask) parameters.moving_mask = new MovingMaskInterpolatorType (*moving_mask);
              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              std::cout << evaluate.size() << " asdf size " << std::endl;
              Math::GradientDescent<Metric::Evaluate<MetricType, ParamType> > optim (evaluate);

              optim.run (max_iter_[iter]);
            }
          }

      protected:
        std::vector<int> max_iter_;
    };

  }
}

#endif
