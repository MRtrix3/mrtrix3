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

#include "algo/random_threaded_loop.h"
#include "algo/random_loop.h"
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

        template <class MetricType, typename U = void>
        struct metric_requires_initialisation {
          typedef int no;
        };

        template <class MetricType>
        struct metric_requires_initialisation<MetricType, typename Void2<typename MetricType::requires_initialisation>::type> {
          typedef int yes;
        };
      }
      //! \endcond

      template <class MetricType, class ParamType>
        class Evaluate {
          public:

            typedef typename ParamType::TransformParamType TransformParamType;
            typedef default_type value_type;

            template <class U = MetricType>
            Evaluate (const MetricType& metric_, ParamType& parameters, typename metric_requires_initialisation<U>::yes = 0) :
              metric (metric_),
              params (parameters),
              iteration (1) {
                // update number of volumes
                metric.init (parameters.im1_image, parameters.im2_image);
            }

            template <class U = MetricType>
            Evaluate (const MetricType& metric, ParamType& parameters, typename metric_requires_initialisation<U>::no = 0) :
              metric (metric),
              params (parameters),
              iteration (1) { }

            //  metric_requires_precompute<U>::yes: operator() loops over processed_image instead of midway_image
            template <class U = MetricType>
            default_type operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient, typename metric_requires_precompute<U>::yes = 0) {
              Eigen::VectorXd overall_cost_function = Eigen::VectorXd::Zero(1,1);
              gradient.setZero();
              params.transformation.set_parameter_vector(x);

              if (directions.cols()) {
                DEBUG ("Reorienting FODs...");
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
                  ThreadedLoop (params.processed_image, 0, 3).run (kernel);
              }
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function.transpose()));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              return overall_cost_function(0);
            }

            struct ThreadFunctor {
              public:
                ThreadFunctor (
                    const std::vector<size_t>& inner_axes,
                    const default_type density,
                    const MetricType& metric,
                    const ParamType& parameters,
                    Eigen::VectorXd& overall_cost_function,
                    Eigen::VectorXd& overall_grad,
                    Math::RNG& rng_engine) :
                  inner_axes (inner_axes),
                  density (density),
                  metric (metric),
                  params (parameters),
                  cost_function (overall_cost_function.size()),
                  gradient (overall_grad.size()),
                  overall_cost_function (overall_cost_function),
                  overall_gradient (overall_grad),
                  rng (rng_engine)  {
                    gradient.setZero();
                    cost_function.setZero();
                    assert(inner_axes.size() >= 2);
                    assert(overall_cost_function.size() == 1);
                  }

                ~ThreadFunctor () {
                  overall_cost_function += cost_function;
                  overall_gradient += gradient;
                }

                void operator() (const Iterator& iter) {
                  auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(rng.get_seed())};
                  auto kern = ThreadKernel<MetricType, ParamType> (metric, params, cost_function, gradient);
                  Iterator iterator (iter);
                  assign_pos_of(iter).to(iterator);
                  auto inner_loop = Random_loop<Iterator, std::default_random_engine>(iterator, engine, inner_axes[1], (float) iterator.size(inner_axes[1]) * density);
                  for (auto j = inner_loop; j; ++j)
                    for (auto k = Loop (inner_axes[0]) (iterator); k; ++k){
                      DEBUG (str(iterator));
                      kern (iterator);
                    }
                }
              protected:
                std::vector<size_t> inner_axes;
                default_type density;
                MetricType metric;
                ParamType params;
                Eigen::VectorXd cost_function;
                Eigen::VectorXd gradient;
                Eigen::VectorXd& overall_cost_function;
                Eigen::VectorXd& overall_gradient;
                Math::RNG rng;
            };

            template <class TransformType_>
              void
              estimate (TransformType_&& trafo,
                  const MetricType& metric,
                  const ParamType& params,
                  Eigen::VectorXd& cost,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
                  const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x) {

                if (params.loop_density < 1.0){
                  DEBUG("stochastic gradient descent, density: " + str(params.loop_density));
                  if (params.robust_estimate){
                    throw Exception ("TODO robust estimate not implemented");
                  } else {
                    Math::RNG rng;
                    gradient.setZero();
                    auto loop = ThreadedLoop (params.midway_image, 0, 3, 2);
                    ThreadFunctor functor (loop.inner_axes, params.loop_density, metric, params, cost, gradient, rng);
                    loop.run_outer (functor);
                  }
                }
                else {
                  ThreadKernel<MetricType, ParamType> kernel (metric, params, cost, gradient);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
                }
              }

            template <class U = MetricType>
            default_type operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient, typename metric_requires_precompute<U>::no = 0) {

              Eigen::VectorXd overall_cost_function = Eigen::VectorXd::Zero(1,1);
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
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function.transpose()));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              return overall_cost_function(0);
            }

            size_t size() {
              return params.transformation.size();
            }

            default_type init (Eigen::VectorXd& x) {
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
