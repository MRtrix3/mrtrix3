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
#include "math/rng.h"

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

          void set_directions (Math::Matrix<float>& dir) {
            directions = dir;
          }

          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType>
          void run (
            MetricType& metric,
            TransformType& transform,
            MovingVoxelType& moving_vox,
            TemplateVoxelType& template_vox) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, TransformType, MovingVoxelType, TemplateVoxelType, BogusMaskType, BogusMaskType >
                (metric, transform, moving_vox, template_vox, NULL, NULL);
            }

          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType, class TemplateMaskType>
          void run_template_mask (
            MetricType& metric,
            TransformType& transform,
            MovingVoxelType& moving_vox,
            TemplateVoxelType& template_vox,
            Ptr<TemplateMaskType>& template_mask) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, TransformType, MovingVoxelType, TemplateVoxelType, BogusMaskType, TemplateMaskType >
                (metric, transform, moving_vox, template_vox, NULL, template_mask);
            }


          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType, class MovingMaskType>
          void run_moving_mask (
            MetricType& metric,
            TransformType& transform,
            MovingVoxelType& moving_vox,
            TemplateVoxelType& template_vox,
            Ptr<MovingMaskType>& moving_mask) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              run_masked<MetricType, TransformType, MovingVoxelType, TemplateVoxelType, MovingMaskType, BogusMaskType >
                (metric, transform, moving_vox, template_vox, moving_mask, NULL);
            }

          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType, class MovingMaskType, class TemplateMaskType>
          void run_masked (
            MetricType& metric,
            TransformType& transform,
            MovingVoxelType& moving_vox,
            TemplateVoxelType& template_vox,
            Ptr<MovingMaskType>& moving_mask,
            Ptr<TemplateMaskType>& template_mask) {

              if (max_iter.size() == 1)
                max_iter.resize (scale_factor.size(), max_iter[0]);
              else if (max_iter.size() != scale_factor.size())
                throw Exception ("the max number of iterations needs to be defined for each multi-resolution level");

              if (init_type == Transform::Init::mass)
                Transform::Init::initialise_using_image_mass (moving_vox, template_vox, transform);
              else if (init_type == Transform::Init::geometric)
                Transform::Init::initialise_using_image_centres (moving_vox, template_vox, transform);

              typedef typename MovingMaskType::voxel_type MovingMaskVoxelType;
              typedef typename TemplateMaskType::voxel_type TemplateMaskVoxelType;

              typedef Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > MovingImageInterpolatorType;

              typedef Metric::Params<TransformType,
                                     Image::BufferScratch<float>::voxel_type,
                                     MovingImageInterpolatorType,
                                     Image::BufferScratch<float>::voxel_type,
                                     Image::Interp::Nearest<MovingMaskVoxelType >,
                                     Image::Interp::Nearest<TemplateMaskVoxelType > > ParamType;

              Math::Vector<typename TransformType::ParameterType> optimiser_weights;
              transform.get_optimiser_weights (optimiser_weights);

              for (size_t level = 0; level < scale_factor.size(); level++) {

                CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

                Image::Filter::Resize moving_resize_filter (moving_vox);
                moving_resize_filter.set_scale_factor (scale_factor[level]);
                moving_resize_filter.set_interp_type (1);
                Image::BufferScratch<float> moving_resized (moving_resize_filter.info());
                Image::BufferScratch<float>::voxel_type moving_resized_vox (moving_resized);
                Image::Filter::GaussianSmooth<float> moving_smooth_filter (moving_resized_vox);

                Image::BufferScratch<float> moving_resized_smoothed (moving_smooth_filter.info());
                Image::BufferScratch<float>::voxel_type moving_resized_smoothed_vox (moving_resized_smoothed);

                Image::Filter::Resize template_resize_filter (template_vox);
                template_resize_filter.set_scale_factor (scale_factor[level]);
                template_resize_filter.set_interp_type (1);
                Image::BufferScratch<float> template_resized (template_resize_filter.info());
                Image::BufferScratch<float>::voxel_type template_resized_vox (template_resized);
                Image::Filter::GaussianSmooth<float> template_smooth_filter (template_resized_vox);
                Image::BufferScratch<float> template_resized_smoothed (template_smooth_filter.info());
                Image::BufferScratch<float>::voxel_type template_resized_smoothed_vox (template_resized_smoothed);

                {
                  LogLevelLatch log_level (0);
                  moving_resize_filter (moving_vox, moving_resized_vox);
                  moving_smooth_filter (moving_resized_vox, moving_resized_smoothed_vox);
                  template_resize_filter (template_vox, template_resized_vox);
                  template_smooth_filter (template_resized_vox, template_resized_smoothed_vox);
                }
                metric.set_moving_image (moving_resized_smoothed_vox);
                ParamType parameters (transform, moving_resized_smoothed_vox, template_resized_smoothed_vox);

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
                if (directions.is_set())
                  evaluate.set_directions (directions);

                Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>,
                                      typename TransformType::UpdateType > optim (evaluate, *transform.get_gradient_descent_updator());


                optim.precondition (optimiser_weights);
                optim.run (max_iter[level], 1.0e-3);
                std::cerr << std::endl;
                parameters.transformation.set_parameter_vector (optim.state());

                //Math::Vector<double> params = optim.state();
                //VAR(optim.function_evaluations());
                //Math::check_function_gradient (evaluate, params, 0.0001, true, optimiser_weights);
              }
            }

        protected:
          std::vector<int> max_iter;
          std::vector<float> scale_factor;
          Transform::Init::InitType init_type;
          Math::Matrix<float> directions;

      };
    }
  }
}

#endif
