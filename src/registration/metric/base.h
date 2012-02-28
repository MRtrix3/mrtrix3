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

#ifndef __mean_squared_metric_h__
#define __mean_squared_metric_h__

namespace MR
{
  namespace Registration
  {
    namespace Metric {

      template <class FixedImageVoxelType, class MovingImageVoxelType, class FixedMaskVoxelType, class MovingMaskVoxelType, class TransformType>
        class BaseParams {
          public:

            BaseParams (
                FixedImageVoxelType& fixed_image,
                MovingImageVoxelType& moving_image,
                TransformType& transform) :
              fixed_image_(fixed_image),
              moving_image_(moving_image),
              transform_(transform) { }

            FixedImageVoxelType fixed_image;
            MovingImageVoxelType moving_image;
            TransformType transform;
            Ptr<FixedMaskVoxelType> fixed_mask;
            Ptr<MovingMaskVoxelType> moving_mask;
            Point<float> point_in_moving;
        };


      class MyMetric {
        public:
          MyMetric () { }

          template <class Param>
            float operator() (Param& param, Math::Vector<float>& gradient) {
              gradient += my_gradient_at_this_point();
              return param.fixed_image.value() - param.moving_image.value();
            }

        protected:

      };


      template <class MetricType, class ParamType> 
        class ThreadKernel {
          public:
            ThreadKernel (const MetricType& metric, ParamType& parameters, double& overall_cost_function, Math::Vector<float>& overall_gradient) :
              metric_ (metric),
              param_ (parameters),
              overall_cost_function_ (overall_cost_function),
              overall_gradient_ (overall_gradient) { }

            ~ThreadKernel () {
              overall_cost_function_ += cost_function_;
              overall_gradient_ += gradient_;
            }

            void operator() (const Iterator& iter) {
              if (param_.fixed_mask) {
                voxel_assign (*param_.fixed_mask, iter);
                if (param_.fixed_mask->value() < 0.5) 
                  return;
              }

              voxel_assign (*param_.fixed_image, iter);
              // .... //
              cost_function_ += metric_ (param_, gradient_);
            }

            protected:
              MetricType metric_;
              ParamType param_;

              double cost_function_;
              Math::Vector<float> gradient_;
        };



      template <class MetricType, class ParamType>
        class Evaluate {
          public:
            Evaluate (const MetricType& metric, ParamType& parameters) :
              metric_ (metric),
              param_ (parameters) { }

            float operator() (const Math::Vector<float>& x, Math::Vector<float>& gradient) {
              double overall_cost_function = 0.0;
              gradient.zero();
              ThreadKernel<MetricType, ParamType> kernel (metric_, param_, overall_cost_function, gradient); 
              Image::threaded_loop (kernel, param_.fixed_image, 2, 0, 3);
              return overall_cost_function;
            }


            float init (Math::Vector<float>& x) { // return initial step size
            }

          protected:
              MetricType metric_;
              ParamType param_;
        };



      class Registration {
        public:
          Registration () { }


          template <class MetricType, class FixedImageVoxelType, class MovingImageVoxelType, class TransformType>
            void run (
                const MetricType& metric, 
                const FixedImageVoxelType& fixed_image, 
                const MovingImageVoxelType& moving_image, 
                const TransformType& transform) {
              run_masked<MetricType,FixedImageVoxelType,MovingImageVoxelType,BogusVoxelType,BogusVoxelType,TransformType> (metric, fixed_image, NULL, moving_image, NULL, transform);
            }



          template <class MetricType, class FixedImageVoxelType, class MovingImageVoxelType, class FixedMaskVoxelType, class MovingMaskVoxelType, class TransformType>
            void run_masked (
                const MetricType& metric, 
                const FixedImageVoxelType& fixed_image, 
                const FixedMaskVoxelType* fixed_mask,
                const MovingImageVoxelType& moving_image, 
                const MovingMaskVoxelType* moving_mask,
                const TransformType& transform) {

              typedef Image::ScratchBuffer<float>::voxel_type BogusVoxelType;
              typedef BaseParams<FixedImageVoxelType,MovingImageVoxelType,FixedMaskVoxelType,MovingMaskVoxelType,TransformType> ParamType;
              ParamType parameters (fixed_image, moving_image, transform);
              if (fixed_mask) parameters.fixed_mask = new FixedMaskVoxelType (*fixed_mask);
              if (moving_mask) parameters.moving_mask = new MovingMaskVoxelType (*moving_mask);
              Evaluate<MetricType, ParamType> evaluate (metric, parameters);

              Math::GradientDescent<Evaluate<MetricType, ParamType> > optim (evaluate);

              optim.run (max_iterations);
            }

          size_t max_iterations;

      };



    }

  }
}

#endif
