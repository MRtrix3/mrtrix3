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

#ifndef __registration_transform_global_search_h__
#define __registration_transform_global_search_h__

#include <vector>
#include <iostream>     // std::streambuf
#include "image.h"
// #include "image/average_space.h"
// #include "filter/normalise.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "adapter/reslice.h"
#include "algo/threaded_loop.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "registration/metric/params.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/evaluate.h"
#include "registration/transform/initialiser.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "math/rng.h"
#include "math/math.h"


namespace MR
{
  namespace Registration
  {

    class GlobalSearch
    {

      public:

        GlobalSearch () :
          max_iter (500),
          scale_factor (0.5),
          pool_size(7),
          loop_density (1.0),
          smooth_factor (1.0),
          grad_tolerance(1.0e-6),
          step_tolerance(1.0e-10),
          log_stream(nullptr),
          init_type (Transform::Init::mass) {
        }

        void set_init_type (Transform::Init::InitType type) {
          init_type = type;
        }

        void set_log_stream (std::streambuf* stream) {
          log_stream = stream;
        }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType>
        void run (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image) {
            typedef Image<float> BogusMaskType;
            run_masked<MetricType, TransformType, Im1ImageType, Im2ImageType, BogusMaskType, BogusMaskType >
              (metric, transform, im1_image, im2_image, nullptr, nullptr);
          }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im2MaskType>
        void run_im2_mask (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          std::unique_ptr<Im2MaskType>& im2_mask) {
            typedef Image<float> BogusMaskType;
            run_masked<MetricType, TransformType, Im1ImageType, Im2ImageType, BogusMaskType, Im2MaskType >
              (metric, transform, im1_image, im2_image, nullptr, im2_mask);
          }


        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType>
        void run_im1_mask (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          std::unique_ptr<Im1MaskType>& im1_mask) {
            typedef Image<float> BogusMaskType;
            run_masked<MetricType, TransformType, Im1ImageType, Im2ImageType, Im1MaskType, BogusMaskType >
              (metric, transform, im1_image, im2_image, im1_mask, nullptr);
          }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
        void run_masked (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          Im1MaskType& im1_mask,
          Im2MaskType& im2_mask) {

            Filter::Smooth im1_smooth_filter (im1_image);
            im1_smooth_filter.set_stdev(smooth_factor * 1.0 / (2.0 * scale_factor));
            auto im1_smoothed = Image<typename Im1ImageType::value_type>::scratch (im1_smooth_filter);

            Filter::Smooth im2_smooth_filter (im2_image);
            im2_smooth_filter.set_stdev(smooth_factor * 1.0 / (2.0 * scale_factor)) ;
            auto im2_smoothed = Image<typename Im2ImageType::value_type>::scratch (im2_smooth_filter);

            Filter::Resize im1_resize_filter (im1_image);
            im1_resize_filter.set_scale_factor (scale_factor);
            im1_resize_filter.set_interp_type (1);
            auto im1_resized = Image<typename Im1ImageType::value_type>::scratch (im1_resize_filter);

            Filter::Resize im2_resize_filter (im2_image);
            im2_resize_filter.set_scale_factor (scale_factor);
            im2_resize_filter.set_interp_type (1);
            auto im2_resized = Image<typename Im1ImageType::value_type>::scratch (im2_resize_filter);

            {
              // LogLevelLatch log_level (0);
              im1_smooth_filter (im1_image, im1_smoothed);
              im2_smooth_filter (im2_image, im2_smoothed);
              im1_resize_filter (im1_smoothed, im1_resized);
              im2_resize_filter (im2_smoothed, im2_resized);
            }

            // TODO

            INFO("global search done.");
          }

      protected:
        size_t max_iter;
        default_type scale_factor;
        size_t pool_size;
        default_type loop_density;
        default_type smooth_factor;
        std::vector<size_t> kernel_extent;
        default_type grad_tolerance;
        default_type step_tolerance;
        std::streambuf* log_stream;
        Transform::Init::InitType init_type;
        size_t gd_repetitions;

    };
  }
}

#endif
