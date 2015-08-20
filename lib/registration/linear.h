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

#include "image.h"
#include "filter/resize.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "registration/metric/params.h"
#include "registration/metric/evaluate.h"
#include "registration/transform/initialiser.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "math/rng.h"

#include <iostream>     // std::streambuf

namespace MR
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
          kernel_extent(3),
          grad_tolerance(1.0e-6),
          step_tolerance(1.0e-10),
          log_stream(nullptr),
          init_type (Transform::Init::mass) {
          scale_factor[0] = 0.5;
          scale_factor[1] = 1;
          kernel_extent[0] = 1;
          kernel_extent[1] = 1;
          kernel_extent[2] = 1;
        }

        void set_max_iter (const std::vector<int>& maxiter) {
          for (size_t i = 0; i < maxiter.size (); ++i)
            if (maxiter[i] < 0)
              throw Exception ("the number of iterations must be positive");
          max_iter = maxiter;
        }

        void set_scale_factor (const std::vector<default_type>& scalefactor) {
          for (size_t level = 0; level < scalefactor.size(); ++level) {
            if (scalefactor[level] <= 0 || scalefactor[level] > 1)
              throw Exception ("the scale factor for each multi-resolution level must be between 0 and 1");
          }
          scale_factor = scalefactor;
        }

        void set_extent (const std::vector<size_t> extent) {
         // TODO: check input
          for (size_t d = 0; d < extent.size(); ++d) {
            if (extent[d] < 1)
          throw Exception ("the neighborhood kernel extent must be at least 1 voxel");
          }
          kernel_extent = extent;
        }

        void set_init_type (Transform::Init::InitType type) {
          init_type = type;
        }

        void set_transform_type (Transform::Init::InitType type) {
          init_type = type;
        }

        void set_directions (Eigen::MatrixXd& dir) {
          directions = dir;
        }

        void set_grad_tolerance (const float& tolerance) {
          grad_tolerance = tolerance;
        }

        void set_gradient_descent_log_stream (std::streambuf* stream) {
          log_stream = stream;
        }

        template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType>
        void run (
          MetricType& metric,
          TransformType& transform,
          MovingImageType& moving_image,
          TemplateImageType& template_image) {
            typedef Image<bool> BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageType, TemplateImageType, BogusMaskType, BogusMaskType >
              (metric, transform, moving_image, template_image, nullptr, nullptr);
          }

        template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType, class TemplateMaskType>
        void run_template_mask (
          MetricType& metric,
          TransformType& transform,
          MovingImageType& moving_image,
          TemplateImageType& template_image,
          std::unique_ptr<TemplateMaskType>& template_mask) {
            typedef Image<bool> BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageType, TemplateImageType, BogusMaskType, TemplateMaskType >
              (metric, transform, moving_image, template_image, nullptr, template_mask);
          }


        template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType, class MovingMaskType>
        void run_moving_mask (
          MetricType& metric,
          TransformType& transform,
          MovingImageType& moving_image,
          TemplateImageType& template_image,
          std::unique_ptr<MovingMaskType>& moving_mask) {
            typedef Image<bool> BogusMaskType;
            run_masked<MetricType, TransformType, MovingImageType, TemplateImageType, MovingMaskType, BogusMaskType >
              (metric, transform, moving_image, template_image, moving_mask, nullptr);
          }

        template <class MetricType, class TransformType, class MovingImageType, class TemplateImageType, class MovingMaskType, class TemplateMaskType>
        void run_masked (
          MetricType& metric,
          TransformType& transform,
          MovingImageType& moving_image,
          TemplateImageType& template_image,
          MovingMaskType& moving_mask,
          TemplateMaskType& template_mask) {

            if (max_iter.size() == 1)
              max_iter.resize (scale_factor.size(), max_iter[0]);
            else if (max_iter.size() != scale_factor.size())
              throw Exception ("the max number of iterations needs to be defined for each multi-resolution level");

            if (init_type == Transform::Init::mass)
              Transform::Init::initialise_using_image_mass (moving_image, template_image, transform);
            else if (init_type == Transform::Init::geometric)
              Transform::Init::initialise_using_image_centres (moving_image, template_image, transform);

            typedef Interp::SplineInterp<MovingImageType, Math::UniformBSpline<typename MovingImageType::value_type>, Math::SplineProcessingType::ValueAndGradient> MovingImageInterpolatorType;

            typedef Metric::Params<TransformType,
                                   MovingImageType,
                                   MovingImageInterpolatorType,
                                   TemplateImageType,
                                   Interp::Nearest<MovingMaskType>,
                                   Interp::Nearest<MovingMaskType> > ParamType;

            Eigen::Matrix<typename TransformType::ParameterType, Eigen::Dynamic, 1> optimiser_weights = transform.get_optimiser_weights();

            for (size_t level = 0; level < scale_factor.size(); level++) {

              CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));


              // TODO: When we go symmetric, and change the gradient calculation to use the UniformBSline interpolation on the fly,
              // lets ditch the resize of moving and "template", and just smooth. We can then get the value of the image as the same time as the gradient for little extra cost.
              // Note that we will still need to resize the 'halfway' template grid. Maybe we should rename the input images to input1 and input2 to avoid confusion.

              Filter::Resize moving_resize_filter (moving_image);
              moving_resize_filter.set_scale_factor (scale_factor[level]);
              moving_resize_filter.set_interp_type (1);
              auto moving_resized = Image<float>::scratch (moving_resize_filter);
              Filter::Smooth moving_smooth_filter (moving_resized);

              auto moving_resized_smoothed = Image<float>::scratch (moving_smooth_filter);

              Filter::Resize template_resize_filter (template_image);
              template_resize_filter.set_scale_factor (scale_factor[level]);
              template_resize_filter.set_interp_type (1);
              auto template_resized = Image<float>::scratch (template_resize_filter);
              Filter::Smooth template_smooth_filter (template_resized);
              auto template_resized_smoothed = Image<float>::scratch (template_smooth_filter);

              {
                LogLevelLatch log_level (0);
                // TODO check this. Shouldn't we be smoothing then resizing? DR: No, smoothing automatically happens within resize. We can probably remove smoothing when using the bspline cubic gradient interpolator
                moving_resize_filter (moving_image, moving_resized);
                moving_smooth_filter (moving_resized, moving_resized_smoothed);
                template_resize_filter (template_image, template_resized);
                template_smooth_filter (template_resized, template_resized_smoothed);
              }
              ParamType parameters (transform, moving_resized_smoothed, template_resized_smoothed);

              INFO ("neighbourhood kernel extent: " +str(kernel_extent));
              parameters.set_extent (kernel_extent);

              if (moving_mask.valid())
                parameters.moving_mask_interp.reset (new Interp::Nearest<MovingMaskType> (moving_mask));
              if (template_mask.valid())
                parameters.template_mask_interp.reset (new Interp::Nearest<TemplateMaskType> (template_mask));

              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              if (directions.cols())
                evaluate.set_directions (directions);

              Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>,
                                    typename TransformType::UpdateType > optim (evaluate, *transform.get_gradient_descent_updator());
              // GradientDescent (Function& function, UpdateFunctor update_functor = LinearUpdate(), value_type step_size_upfactor = 3.0, value_type step_size_downfactor = 0.1)

              optim.precondition (optimiser_weights);
              optim.run (max_iter[level], grad_tolerance, false, step_tolerance, 1e-10, 1e-10, log_stream);
              parameters.transformation.set_parameter_vector (optim.state());

              if (log_stream){
                std::ostream log_os(log_stream);
                log_os << "\n\n"; // two empty lines for gnuplot's index recognition
              }

              //Math::Vector<double> params = optim.state();
              //VAR(optim.function_evaluations());
              //Math::check_function_gradient (evaluate, params, 0.0001, true, optimiser_weights);
            }
          }

      protected:
        std::vector<int> max_iter;
        std::vector<default_type> scale_factor;
        std::vector<size_t> kernel_extent;
        default_type grad_tolerance;
        default_type step_tolerance;
        std::streambuf* log_stream;
        Transform::Init::InitType init_type;
        Eigen::MatrixXd directions;

    };
  }
}

#endif
