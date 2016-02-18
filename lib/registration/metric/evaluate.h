/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __registration_metric_evaluate_h__
#define __registration_metric_evaluate_h__

// #include "algo/stochastic_threaded_loop.h"
#include "algo/random_threaded_loop.h"
#include "algo/random_loop.h"
#include "registration/metric/thread_kernel.h"
#include "algo/threaded_loop.h"
#include "registration/transform/reorient.h"
#include "image.h"
#include "timer.h"

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

        // template<typename T> // this does not compule with clang, fine with gcc
        // struct has_robust_estimator
        // {
        // private:
        //   typedef std::true_type yes;
        //   typedef std::false_type no;
        //   Eigen::Matrix<default_type, Eigen::Dynamic, 1> mat;
        //   std::vector<Eigen::Matrix<default_type, Eigen::Dynamic, 1>> vec;
        //   template<typename U> static auto test(bool) -> decltype(std::declval<U>().robust_estimate(mat, vec) == 1, yes());
        //   template<typename> static no test(...);
        // public:
        //   static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
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
              iteration (1) {
            }

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

              if (directions.cols()) {
                INFO ("Reorienting FODs...");
                std::shared_ptr<Image<default_type> > im1_image_reoriented;
                std::shared_ptr<Image<default_type> > im2_image_reoriented;
                im1_image_reoriented = std::make_shared<Image<default_type>>(Image<default_type>::scratch (params.im1_image));
                im2_image_reoriented = std::make_shared<Image<default_type>>(Image<default_type>::scratch (params.im2_image));
                Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions);
                Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions);
                params.set_im1_iterpolator (*im1_image_reoriented);
                params.set_im2_iterpolator (*im2_image_reoriented);
              }

              metric.precompute (params);
              {
                ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
              }
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " +str(overall_cost_function));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              return overall_cost_function;

            }

            // template <class MetricType, class ParamType>
            struct ThreadFunctor {
              public:
                ThreadFunctor (
                    const std::vector<size_t>& inner_axes,
                    const default_type density,
                    const MetricType& metric, const ParamType& parameters,
                    default_type& overall_cost_function, Eigen::VectorXd& overall_grad,
                    Math::RNG& rng_engine) :
                  // kern (kernel),
                  inner_axes (inner_axes),
                  density (density),
                  metric (metric),
                  params (parameters),
                  cost_function (0.0),
                  gradient (overall_grad.size()), // BUG: set zero
                  overall_cost_function (overall_cost_function),
                  overall_gradient (overall_grad),
                  rng (rng_engine)  {
                    gradient.setZero();
                    assert(inner_axes.size() >= 2); }

                ~ThreadFunctor () {
                  // WARN("~ThreadFunctor");
                  // VAR(gradient.transpose());
                  overall_cost_function += cost_function;
                  overall_gradient += gradient;
                }

                void operator() (const Iterator& iter) {
                  auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(rng.get_seed())};
                  // DEBUG(str(iter));
                  auto kern = ThreadKernel<MetricType, ParamType> (metric, params, overall_cost_function, gradient);
                  Iterator iterator (iter);
                  assign_pos_of(iter).to(iterator);
                  auto inner_loop = Random_loop<Iterator, std::default_random_engine>(iterator, engine, inner_axes[1], (float) iterator.size(inner_axes[1]) * density);
                  for (auto j = inner_loop; j; ++j)
                    for (auto k = Loop (inner_axes[0]) (iterator); k; ++k){
                      // cnt += 1;
                      DEBUG(str(iterator));
                      kern(iterator);
                    }
                }
              protected:
                // typename std::remove_reference<Kernel>::type kern;
                std::vector<size_t> inner_axes;
                default_type density;
                MetricType metric;
                ParamType params;
                default_type cost_function;
                Eigen::VectorXd gradient;
                default_type& overall_cost_function;
                Eigen::VectorXd& overall_gradient;
                Math::RNG rng;
                // ThreadKernel<MetricType, ParamType> kern;
            };

            template <class TransformType_>
              // typename std::enable_if<has_robust_estimator<TransformType_>::value, void>::type // doesn't work with clang
              void
              estimate (TransformType_&& trafo,
                  const MetricType& metric,
                  const ParamType& params,
                  default_type& cost,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
                  const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x) {

                auto timer = Timer ();
                if (params.loop_density < 1.0){
                  // Eigen::Matrix<default_type, Eigen::Dynamic, 1> optimiser_weights = trafo.get_optimiser_weights();
                  DEBUG("stochastic gradient descent, density: " + str(params.loop_density));
                  if (params.robust_estimate){
                    DEBUG("robust estimate");
                    size_t n_estimates = 5;
                    default_type density = params.loop_density / (default_type) n_estimates;
                    DEBUG(str("density: " + str(density)));
                    std::vector<Eigen::Matrix<default_type, Eigen::Dynamic, 1>> grad_estimates(n_estimates);
                    for (size_t i = 0; i < n_estimates; i++) {
                      Eigen::VectorXd gradient_estimate(gradient.size());
                      gradient_estimate.setZero();
                      Math::RNG rng;
                      auto loop = ThreadedLoop (params.midway_image, 0, 3, 2);
                      ThreadFunctor functor (loop.inner_axes, density, metric, params, cost, gradient_estimate, rng); // <MetricType, ParamType>
                      assert (gradient_estimate.isApprox(gradient_estimate));
                      // VAR(gradient_estimate.transpose());
                      loop.run_outer (functor);
                      DEBUG("elapsed: (" + str(i) + ") " + str(timer.elapsed()));
                      // StochasticThreadedLoop (params.midway_image, 0, 3).run (kernel, params.loop_density / (default_type) n_estimates);
                      grad_estimates[i] = gradient_estimate;
                      // VAR(grad_estimates[i].transpose());
                    }
                    // define weiszfeld median precision and maximum number of iterations
                    auto m = std::min( {float(params.midway_image.spacing(0)), float(params.midway_image.spacing(1)), float(params.midway_image.spacing(2))} );
                    default_type precision = 1.0e-6 * m;
                    //TODO: use actual learning rate
                    default_type learning_rate = 1.0;
                    params.transformation.robust_estimate(gradient, grad_estimates, params.control_points, x, precision, 1000, learning_rate);
                    // VAR(gradient.transpose());
                  } else {
                    // std::vector<size_t> dimensions(3);
                    // dimensions[0] = params.midway_image.size(0);
                    // dimensions[1] = params.midway_image.size(1);
                    // dimensions[2] = params.midway_image.size(2);
                    // ThreadKernel<MetricType, ParamType> kernel (metric, params, cost, gradient);
                    // RandomThreadedLoop (params.midway_image, 0, 3).run (kernel, params.loop_density, dimensions);
                    Math::RNG rng;
                    gradient.setZero();
                    auto loop = ThreadedLoop (params.midway_image, 0, 3, 2);
                    ThreadFunctor functor (loop.inner_axes, params.loop_density, metric, params, cost, gradient, rng); // <MetricType, ParamType>
                    loop.run_outer (functor);
                  }
                }
                else {
                  ThreadKernel<MetricType, ParamType> kernel (metric, params, cost, gradient);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
                }
              }

            // template <class TransformType_>
            //   typename std::enable_if<!has_robust_estimator<TransformType_>::value, void>::type
            //   estimate (TransformType_&& trafo,
            //       MetricType& metric,
            //       ParamType& params,
            //       default_type& cost,
            //       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
            //       const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x) {
            //     if (params.robust_estimate) WARN("metric is not robust");
            //     if (params.sparsity > 0.0){
            //       ThreadKernel<MetricType, ParamType> kernel (metric, params, cost, gradient);
            //       INFO("StochasticThreadedLoop " + str(params.sparsity));
            //       StochasticThreadedLoop (params.midway_image, 0, 3).run (kernel, params.sparsity);
            //     } else {
            //       ThreadKernel<MetricType, ParamType> kernel (metric, params, cost, gradient);
            //       ThreadedLoop (params.midway_image, 0, 3).run (kernel);
            //     }
            //   }

            template <class U = MetricType>
            double operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient, typename metric_requires_precompute<U>::no = 0) {

              default_type overall_cost_function = 0.0;
              gradient.setZero();
              params.transformation.set_parameter_vector(x);

              if (directions.cols()) {
                INFO ("Reorienting FODs...");
                std::shared_ptr<Image<default_type> > im1_image_reoriented;
                std::shared_ptr<Image<default_type> > im2_image_reoriented;
                im1_image_reoriented = std::make_shared<Image<default_type>>(Image<default_type>::scratch (params.im1_image));
                im2_image_reoriented = std::make_shared<Image<default_type>>(Image<default_type>::scratch (params.im2_image));
                Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions);
                Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions);
                params.set_im1_iterpolator (*im1_image_reoriented);
                params.set_im2_iterpolator (*im2_image_reoriented);
              }

              estimate(params.transformation, metric, params, overall_cost_function, gradient, x);
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              return overall_cost_function;
            }

            size_t size() {
              return params.transformation.size();
            }

            double init (Eigen::VectorXd& x) {
              params.transformation.get_parameter_vector(x);
              return 1.0;
            }

            void set_directions (Eigen::MatrixXd& dir) {
              directions = dir;
            }

          protected:
              MetricType metric;
              ParamType params;
              std::vector<size_t> extent;
              size_t iteration;
              Eigen::MatrixXd directions;

      };
    }
  }
}

#endif
