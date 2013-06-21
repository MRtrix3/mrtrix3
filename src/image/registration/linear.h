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

#ifndef __registration_linear_h__
#define __registration_linear_h__

#include <vector>

#include "ptr.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "image/filter/resize.h"
#include "image/interp/linear.h"
#include "image/interp/nearest.h"
#include "image/registration/metric/params.h"
#include "image/registration/metric/evaluate.h"
#include "image/registration/transform/initialiser.h"
#include "math/matrix.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {

      extern const App::OptionGroup rigid_options;
      extern const App::OptionGroup affine_options;
      extern const App::OptionGroup syn_options;
      extern const App::OptionGroup initialisation_options;
      extern const App::OptionGroup fod_options;



      class Linear
      {

        public:

          Linear () :
            max_iter (1, 300),
            scale_factor (2),
            init_type (Transform::Init::mass) {
            scale_factor[0] = 0.5;
            scale_factor[1] = 1;
          }

          void set_max_iter (const std::vector<int>& maxiter) {
            for (size_t i = 0; i < maxiter.size (); ++i)
              if (maxiter[i] < 0)
                throw Exception ("the number of iterations must be positive");
            max_iter = maxiter;
          }

          void set_scale_factor (const std::vector<float>& scalefactor) {
            for (size_t level = 0; level < scalefactor.size(); ++level) {
              if (scalefactor[level] <= 0 || scalefactor[level] > 1)
                throw Exception ("the scale factor for each multi-resolution level must be between 0 and 1");
            }
            scale_factor = scalefactor;
          }

          void set_init_type (Transform::Init::InitType type) {
            init_type = type;
          }

          void set_transform_type (Transform::Init::InitType type) {
            init_type = type;
          }

          template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType>
          void run (
            MetricType& metric,
            TransformType& transform,
            MovingImageType& moving_image,
            TemplateImageType& template_image) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, TransformType, MovingImageType, TemplateImageType, BogusMaskType, BogusMaskType >
                (metric, transform, moving_image, template_image, NULL, NULL);
            }

          template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType, class TemplateMaskType>
          void run_template_mask (
            MetricType& metric,
            TransformType& transform,
            MovingImageType& moving_image,
            TemplateImageType& template_image,
            Ptr<TemplateMaskType>& template_mask) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, TransformType, MovingImageType, TemplateImageType, BogusMaskType, TemplateMaskType >
                (metric, transform, moving_image, template_image, NULL, template_mask);
            }


          template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType, class MovingMaskType>
          void run_moving_mask (
            MetricType& metric,
            TransformType& transform,
            MovingImageType& moving_image,
            TemplateImageType& template_image,
            Ptr<MovingMaskType>& moving_mask) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, TransformType, MovingImageType, TemplateImageType, MovingMaskType, BogusMaskType >
                (metric, transform, moving_image, template_image, moving_mask, NULL);
            }

          template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType, class MovingMaskType, class TemplateMaskType>
          void run_masked (
            MetricType& metric,
            TransformType& transform,
            MovingImageType& moving_image,
            TemplateImageType& template_image,
            Ptr<MovingMaskType>& moving_mask,
            Ptr<TemplateMaskType>& template_mask) {

              typename MovingImageType::voxel_type moving_image_vox (moving_image);
              typename TemplateImageType::voxel_type template_image_vox (template_image);

              if (max_iter.size() == 1)
                max_iter.resize (scale_factor.size(), max_iter[0]);
              else if (max_iter.size() != scale_factor.size())
                throw Exception ("the max number of iterations needs to be defined for each multi-resolution level");

              if (init_type == Transform::Init::mass)
                Transform::Init::initialise_using_image_mass (moving_image_vox, template_image_vox, transform);
              else if (init_type == Transform::Init::geometric)
                Transform::Init::initialise_using_image_centres (moving_image_vox, template_image_vox, transform);

              typedef typename MovingMaskType::voxel_type MovingMaskVoxelType;
              typedef typename TemplateMaskType::voxel_type TemplateMaskVoxelType;

              typedef Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > MovingImageInterpolatorType;

              typedef Metric::Params<TransformType,
                                     MovingImageInterpolatorType,
                                     Image::BufferScratch<float>::voxel_type,
                                     Image::Interp::Nearest<MovingMaskVoxelType >,
                                     Image::Interp::Nearest<TemplateMaskVoxelType > > ParamType;

              for (size_t level = 0; level < scale_factor.size(); level++) {

                CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

                Image::Filter::Resize moving_resize_filter (moving_image_vox);
                Image::Filter::Resize template_resize_filter (template_image_vox);
                moving_resize_filter.set_scale_factor (scale_factor[level]);
                moving_resize_filter.set_interp_type (1);
                Image::BufferScratch<float> moving_resized (moving_resize_filter.info());
                Image::BufferScratch<float>::voxel_type moving_resized_vox (moving_resized);
                Image::Filter::GaussianSmooth<float> moving_smooth_filter (moving_resized_vox);
                Image::BufferScratch<float> moving_resized_smoothed (moving_smooth_filter.info());
                Image::BufferScratch<float>::voxel_type moving_resized_smoothed_vox (moving_resized_smoothed);

                template_resize_filter.set_scale_factor (scale_factor[level]);
                template_resize_filter.set_interp_type (1);
                Image::BufferScratch<float> template_buffer (template_resize_filter.info());
                Image::BufferScratch<float>::voxel_type template_vox (template_buffer);
                {
                  LogLevelLatch log_level (0);
                  moving_resize_filter (moving_image_vox, moving_resized_vox);
                  moving_smooth_filter (moving_resized_vox, moving_resized_smoothed_vox);
                  template_resize_filter (template_image_vox, template_vox);
                }
                MovingImageInterpolatorType moving_interp (moving_resized_smoothed_vox);
                metric.set_moving_image (moving_resized_smoothed_vox);
                ParamType parameters (transform, moving_interp, template_vox);

                Ptr<MovingMaskVoxelType> moving_mask_vox;
                Ptr<TemplateMaskVoxelType> template_mask_vox;
                if (moving_mask) {
                  moving_mask_vox = new MovingMaskVoxelType (*moving_mask);
                  parameters.moving_mask_interp = new Image::Interp::Nearest<MovingMaskVoxelType> (*moving_mask_vox);
                }
                if (template_mask) {
                  template_mask_vox = new TemplateMaskVoxelType (*template_mask);
                  parameters.template_mask_interp = new Image::Interp::Nearest<TemplateMaskVoxelType> (*template_mask_vox);
                }

                Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
                Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>,
                                      typename TransformType::UpdateType > optim (evaluate, *transform.get_gradient_descent_updator());
                Math::Vector<typename TransformType::ParameterType> optimiser_weights;
                parameters.transformation.get_optimiser_weights (optimiser_weights);

                optim.precondition (optimiser_weights);
                optim.run (max_iter[level], 1.0e-4);
                parameters.transformation.set_parameter_vector (optim.state());

  //              Math::Vector<double> params = optim.state();
  //              VAR(optim.function_evaluations());
  //              Math::check_function_gradient(evaluate, params, 0.1, true, optimiser_weights);
              }
            }

        protected:
          std::vector<int> max_iter;
          std::vector<float> scale_factor;
          Transform::Init::InitType init_type;

      };
    }
  }
}

#endif
