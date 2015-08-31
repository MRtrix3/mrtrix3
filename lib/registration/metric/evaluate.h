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

#ifndef __registration_metric_evaluate_h__
#define __registration_metric_evaluate_h__

#include "registration/metric/thread_kernel.h"
#include "algo/threaded_loop.h"
#include "registration/transform/reorient.h"
#include "image.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <class MetricType, class ParamType>
        class Evaluate {
          public:

            typedef typename ParamType::TransformParamType TransformParamType;
            typedef default_type value_type;

            Evaluate (const MetricType& metric, ParamType& parameters) :
              metric (metric),
              params (parameters),
              iteration (1) { }


            double operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              default_type overall_cost_function = 0.0;
              gradient.setZero();
              params.transformation.set_parameter_vector(x);

              std::unique_ptr<Image<float> > reoriented_moving; // MP unused
              std::unique_ptr<Image<float> > reoriented_template; // MP unused

              // TODO I wonder if reorienting within the metric would be quicker since it's only one pass? Means we would need to set current affine to the metric4D before running the thread kernel
//              if (directions.cols()) {
//                reoriented_moving.reset (new Image<float> (Image<float>::scratch (params.moving_image)));
//                Registration::Transform::reorient (params.moving_image, *reoriented_moving, params.transformation.get_matrix(), directions);
//                params.set_moving_iterpolator (*reoriented_moving);
//                metric.set_moving_image (*reoriented_moving);
//              }

              {
                ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                // ThreadedLoop (params.template_image, 0, 3).run (kernel);
                ThreadedLoop (params.midway_image, 0, 3).run (kernel);
              }
              // std::cerr.precision(10);
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " +str(overall_cost_function));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              return overall_cost_function;
            }

            void set_directions (Eigen::MatrixXd& dir) {
              directions = dir;
            }

            size_t size() {
              return params.transformation.size();
            }

            double init (Eigen::VectorXd& x) {
              params.transformation.get_parameter_vector(x);
              return 1.0;
            }

          protected:
              MetricType metric;
              ParamType params;
              Eigen::MatrixXd directions;
              std::vector<size_t> extent;
              size_t iteration;

      };
    }
  }
}

#endif
