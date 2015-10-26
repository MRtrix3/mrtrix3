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

#include "algo/stochastic_threaded_loop.h"
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

      //! \cond skip
      namespace {
        template<class T>
        struct Void2 {
          typedef void type;
        };


        template <class MetricType, typename U = void>
        struct metric_requires_precompute {
          typedef int no;
        };

        template <class MetricType>
        struct metric_requires_precompute<MetricType, typename Void2<typename MetricType::requires_precompute>::type> {
          typedef int yes;
        };

        // template<class T>
        // struct Void3 {
        //   typedef void type;
        // };

        // template <class TransformType, typename U = void>
        // struct transform_has_robust_estimator {
        //   typedef int no;
        // };

        // template <class TransformType>
        // struct transform_has_robust_estimator<TransformType, typename Void3<typename TransformType::has_robust_estimator>::type> {
        //   typedef int yes;
        // };

        // template <typename T>
        // struct has_robust_estimator_method
        // {
        //     struct dummy { /* something */ };

        //     template <typename C, typename P>
        //     static auto test(P * p) -> decltype(std::declval<C>().has_robust_estimator(*p), std::true_type());

        //     template <typename, typename>
        //     static std::false_type test(...);

        //     typedef decltype(test<T, dummy>(nullptr)) type;
        //     static const bool value = std::is_same<std::true_type, decltype(test<T, dummy>(nullptr))>::value;
        // };

      }
      //! \endcond

      template <class MetricType, class ParamType>
        class Evaluate {
          public:

            typedef typename ParamType::TransformParamType TransformParamType;
            typedef default_type value_type;

            Evaluate (const MetricType& metric, ParamType& parameters) :
              metric (metric),
              params (parameters),
              iteration (1) { }

            // template <class U = MetricType>
            // Evaluate (const MetricType& metric, ParamType& parameters, typename metric_requires_precompute<U>::yes = 0) :
            //   metric (metric),
            //   params (parameters),
            //   iteration (1) {
            //     metric.precompute(parameters);
            //     params(parameters); }

            template <class U = MetricType>
            double operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient, typename metric_requires_precompute<U>::yes = 0) {
              default_type overall_cost_function = 0.0;
              gradient.setZero();
              params.transformation.set_parameter_vector(x);

              metric.precompute(params);
              {
                ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
              }
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " +str(overall_cost_function));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              return overall_cost_function;
              // std::unique_ptr<Image<float> > reoriented_moving; // MP unused
              // std::unique_ptr<Image<float> > reoriented_template; // MP unused

              // TODO I wonder if reorienting within the metric would be quicker since it's only one pass? Means we would need to set current affine to the metric4D before running the thread kernel
//              if (directions.cols()) {
//                reoriented_moving.reset (new Image<float> (Image<float>::scratch (params.im1_image)));
//                Registration::Transform::reorient (params.im1_image, *reoriented_moving, params.transformation.get_matrix(), directions);
//                params.set_moving_iterpolator (*reoriented_moving);
//                metric.set_moving_image (*reoriented_moving);
//              }
            }

            // template <class U = TransformParamType>
            // void estimate(Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
            //   default_type overall_cost_function,
            //   typename transform_has_robust_estimator<U>::yes = 0){
            //     WARN("transform_has_robust_estimator<U>::yes"); // TODO: this SFINAE does not work
            //     if (params.sparsity > 0.0){
            //       ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
            //       INFO("StochasticThreadedLoop " + str(params.sparsity));
            //       StochasticThreadedLoop (params.midway_image, 0, 3).run (kernel, params.sparsity);
            //     }
            //     else {
            //       ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
            //       ThreadedLoop (params.midway_image, 0, 3).run (kernel);
            //     }
            // }

            // template <class U = TransformParamType>
            // void estimator(Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
            //   default_type overall_cost_function,
            //   typename transform_has_robust_estimator<U>::no = 0){
            //     if (has_robust_estimator_method<U>::value){
            //       WARN("YEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEES");
            //     }
            //     else {
            //       WARN("NOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
            //     }
            //     if (params.sparsity > 0.0){
            //       ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
            //       INFO("StochasticThreadedLoop " + str(params.sparsity));
            //       StochasticThreadedLoop (params.midway_image, 0, 3).run (kernel, params.sparsity);
            //     }
            //     else {
            //       ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
            //       ThreadedLoop (params.midway_image, 0, 3).run (kernel);
            //     }
            // }



            template <class U = MetricType>
            double operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient, typename metric_requires_precompute<U>::no = 0) {

              default_type overall_cost_function = 0.0;
              gradient.setZero();
              params.transformation.set_parameter_vector(x);
              {
                // estimate(gradient, overall_cost_function);
                if (params.sparsity > 0.0){
                  ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                  INFO("StochasticThreadedLoop " + str(params.sparsity));
                  StochasticThreadedLoop (params.midway_image, 0, 3).run (kernel, params.sparsity);
                }
                else {
                  ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
                }

                // if (params.sparsity > 0.0){
                //   ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                //   INFO("StochasticThreadedLoop " + str(params.sparsity));
                //   StochasticThreadedLoop (params.midway_image, 0, 3).run (kernel, params.sparsity);
                // }
                // else {
                //   ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                //   ThreadedLoop (params.midway_image, 0, 3).run (kernel);
                // }
              }
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function));
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
