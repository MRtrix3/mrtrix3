/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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
        struct Void2 { NOMEMALIGN
          using type = void;
        };

        template <class MetricType, typename U = void>
        struct metric_requires_precompute { NOMEMALIGN
          using no = int;
        };

        template <class MetricType>
        struct metric_requires_precompute<MetricType, typename Void2<typename MetricType::requires_precompute>::type> { NOMEMALIGN
          using yes = int;
        };

        template <class MetricType, typename U = void>
        struct metric_requires_initialisation { NOMEMALIGN
          using no = int;
        };

        template <class MetricType>
        struct metric_requires_initialisation<MetricType, typename Void2<typename MetricType::requires_initialisation>::type> { NOMEMALIGN
          using yes = int;
        };
      }
      //! \endcond

      template <class MetricType, class ParamType>
        class Evaluate { MEMALIGN(Evaluate<MetricType,ParamType>)
          public:

            using TransformParamType = typename ParamType::TransformParamType;
            using value_type = default_type;

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
                im1_image_reoriented = make_shared<Image<default_type>>(Image<default_type>::scratch (params.im1_image));
                im2_image_reoriented = make_shared<Image<default_type>>(Image<default_type>::scratch (params.im2_image));
                {
                   LogLevelLatch log_level (0);
                  Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions);
                  Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions);
                }
                params.set_im1_iterpolator (*im1_image_reoriented);
                params.set_im2_iterpolator (*im2_image_reoriented);
              }

              metric.precompute (params);
              {
                overlap_count = 0;
                ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient, &overlap_count);
                  ThreadedLoop (params.processed_image, 0, 3).run (kernel);
              }
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function.transpose()));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              DEBUG ("  overlapping voxels: " + str(overlap_count));
              return overall_cost_function(0);
            }

            struct ThreadFunctor { MEMALIGN(ThreadFunctor)
              public:
                ThreadFunctor (
                    const vector<size_t>& inner_axes,
                    const default_type density,
                    const MetricType& metric,
                    const ParamType& parameters,
                    Eigen::VectorXd& overall_cost_function,
                    Eigen::VectorXd& overall_grad,
                    Math::RNG& rng_engine,
                    ssize_t* overlap_count = nullptr) :
                  inner_axes (inner_axes),
                  density (density),
                  metric (metric),
                  params (parameters),
                  cost_function (overall_cost_function.size()),
                  gradient (overall_grad.size()),
                  overall_cost_function (overall_cost_function),
                  overall_gradient (overall_grad),
                  rng (rng_engine),
                  overlap_count (overlap_count)  {
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
                  auto kern = ThreadKernel<MetricType, ParamType> (metric, params, cost_function, gradient, overlap_count);
                  Iterator iterator (iter);
                  assign_pos_of(iter).to(iterator);
                  auto inner_loop = Random_loop<Iterator, std::default_random_engine>(iterator, engine, inner_axes[1], (float) iterator.size(inner_axes[1]) * density);
                  for (auto j = inner_loop; j; ++j)
                    for (auto k = Loop (inner_axes[0]) (iterator); k; ++k) {
                      kern (iterator);
                    }
                }
              protected:
                vector<size_t> inner_axes;
                default_type density;
                MetricType metric;
                ParamType params;
                Eigen::VectorXd cost_function;
                Eigen::VectorXd gradient;
                Eigen::VectorXd& overall_cost_function;
                Eigen::VectorXd& overall_gradient;
                Math::RNG rng;
                ssize_t* overlap_count;
            };

            template <class TransformType_>
              void
              estimate (TransformType_&& trafo,
                  const MetricType& metric,
                  const ParamType& params,
                  Eigen::VectorXd& cost,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
                  const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
                  ssize_t* overlap_count = nullptr) {

                if (params.loop_density < 1.0) {
                  DEBUG ("stochastic gradient descent, density: " + str(params.loop_density));
                  if (params.robust_estimate){
                    throw Exception ("TODO robust estimate not implemented");
                  } else {
                    Math::RNG rng;
                    gradient.setZero();
                    auto loop = ThreadedLoop (params.midway_image, 0, 3, 2);
                    if (overlap_count)
                      *overlap_count = 0;
                    ThreadFunctor functor (loop.inner_axes, params.loop_density, metric, params, cost, gradient, rng, overlap_count);
                    loop.run_outer (functor);
                  }
                }
                else {
                  if (overlap_count)
                    *overlap_count = 0;
                  ThreadKernel <MetricType, ParamType> kernel (metric, params, cost, gradient, overlap_count);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
                }
              }

            template <class U = MetricType>
            default_type operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient, typename metric_requires_precompute<U>::no = 0) {

              Eigen::VectorXd overall_cost_function = Eigen::VectorXd::Zero(1,1);
              gradient.setZero();
              params.transformation.set_parameter_vector(x);

              if (directions.cols()) {
                DEBUG ("Reorienting FODs...");
                std::shared_ptr<Image<default_type> > im1_image_reoriented;
                std::shared_ptr<Image<default_type> > im2_image_reoriented;
                im1_image_reoriented = make_shared<Image<default_type>>(Image<default_type>::scratch (params.im1_image));
                im2_image_reoriented = make_shared<Image<default_type>>(Image<default_type>::scratch (params.im2_image));
                {
                   LogLevelLatch log_level (0);
                  Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions);
                  Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions);
                }
                params.set_im1_iterpolator (*im1_image_reoriented);
                params.set_im2_iterpolator (*im2_image_reoriented);
              }

              estimate (params.transformation, metric, params, overall_cost_function, gradient, x, &overlap_count);

              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function.transpose()));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              DEBUG ("  overlapping voxels: " + str(overlap_count));
              return overall_cost_function(0);
            }

            size_t size() {
              return params.transformation.size();
            }

            ssize_t overlap() {
              return overlap_count;
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
              vector<size_t> extent;
              size_t iteration;
              Eigen::MatrixXd directions;
              ssize_t overlap_count;

      };
    }
  }
}

#endif
