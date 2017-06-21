/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __registration_metric_evaluate_h__
#define __registration_metric_evaluate_h__

#include "registration/metric/thread_kernel.h"
#include "algo/threaded_loop.h"
#include "algo/loop.h"
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
            Evaluate () = delete;

            template <class U = MetricType>
            Evaluate (const MetricType& metric_, ParamType& parameters, typename metric_requires_initialisation<U>::yes = 0) :
              metric (metric_),
              params (parameters),
              iteration (1) {
                // update number of volumes
                metric.init (parameters.im1_image, parameters.im2_image);
                metric.set_weights(params.get_weights());
            }

            template <class U = MetricType>
            Evaluate (const MetricType& metric_, ParamType& parameters, typename metric_requires_initialisation<U>::no = 0) :
              metric (metric_),
              params (parameters),
              iteration (1) { metric.set_weights(params.get_weights()); }

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
                //    LogLevelLatch log_level (0); TODO uncomment
                  if (params.mc_settings.size()) {
                    DEBUG ("Tissue contrast specific FOD reorientation");
                    Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions, false, params.mc_settings);
                    Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions, false, params.mc_settings);
                  } else {
                    DEBUG ("FOD reorientation");
                    Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions);
                    Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions);
                  }
                }

                params.set_im1_iterpolator (*im1_image_reoriented);
                params.set_im2_iterpolator (*im2_image_reoriented);
              }

              metric.precompute (params);
              {
                overlap_count = 0;
                ThreadKernel<MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient, &overlap_count);
                {
                  LogLevelLatch log_level (0);
                  ThreadedLoop (params.processed_image, 0, 3).run (kernel);
                }
              }
              DEBUG ("Metric evaluate iteration: " + str(iteration++) + ", cost: " + str(overall_cost_function.transpose()));
              DEBUG ("  x: " + str(x.transpose()));
              DEBUG ("  gradient: " + str(gradient.transpose()));
              DEBUG ("  norm(gradient): " + str(gradient.norm()));
              DEBUG ("  overlapping voxels: " + str(overlap_count));
              return overall_cost_function(0);
            }

            // template <class TransformType_>
            //   void estimate (TransformType_&& trafo,
            //       const MetricType& metric,
            //       const ParamType& params,
            //       Eigen::VectorXd& cost,
            //       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
            //       const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
            //       ssize_t* overlap_count = nullptr) {

            //     if (params.loop_density < 1.0) {
            //       DEBUG ("stochastic gradient descent, density: " + str(params.loop_density));
            //       Math::RNG rng;
            //       gradient.setZero();
            //       auto loop = ThreadedLoop (params.midway_image, 0, 3, 2);
            //       if (overlap_count)
            //         *overlap_count = 0;
            //       StochasticThreadKernel <MetricType, ParamType> functor (loop.inner_axes, params.loop_density, metric, params, cost, gradient, rng, overlap_count);
            //       loop.run_outer (functor);
            //     }
            //     else {
            //       if (overlap_count)
            //         *overlap_count = 0;
            //       ThreadKernel <MetricType, ParamType> kernel (metric, params, cost, gradient, overlap_count);

            //       if (params.robust_estimate_subset) {
            //         assert(params.robust_estimate_subset_from.size());
            //         assert(params.robust_estimate_subset_size.size());
            //         Adapter::Subset<Image<default_type>> midway_subset (params.midway_image, params.robust_estimate_subset_from, params.robust_estimate_subset_size);
            //         ThreadedLoop (midway_subset, 0, 3).run (kernel);
            //       } else {
            //         ThreadedLoop (params.midway_image, 0, 3).run (kernel);
            //       }
            //     }
            //   }

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
                  // LogLevelLatch log_level (0); // TODO uncomment
                  if (params.mc_settings.size()) {
                    DEBUG ("Tissue contrast specific FOD reorientation");
                    Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions, false, params.mc_settings);
                    Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions, false, params.mc_settings);
                  } else {
                    DEBUG ("FOD reorientation");
                    Registration::Transform::reorient (params.im1_image, *im1_image_reoriented, params.transformation.get_transform_half(), directions);
                    Registration::Transform::reorient (params.im2_image, *im2_image_reoriented, params.transformation.get_transform_half_inverse(), directions);
                  }
                }

                params.set_im1_iterpolator (*im1_image_reoriented);
                params.set_im2_iterpolator (*im2_image_reoriented);
              }

              // estimate (params.transformation, metric, params, overall_cost_function, gradient, x, &overlap_count);
              if (params.loop_density < 1.0) {
                DEBUG ("stochastic gradient descent, density: " + str(params.loop_density));
                Math::RNG rng;
                gradient.setZero();
                auto loop = ThreadedLoop (params.midway_image, 0, 3, 2);
                overlap_count = 0;
                StochasticThreadKernel <MetricType, ParamType> functor (loop.inner_axes, params.loop_density, metric, params, overall_cost_function, gradient, rng, &overlap_count);
                {
                  LogLevelLatch log_level (0);
                  loop.run_outer (functor);
                }
              } else {
                overlap_count = 0;
                ThreadKernel <MetricType, ParamType> kernel (metric, params, overall_cost_function, gradient, &overlap_count);
                if (params.robust_estimate_subset) {
                  assert(params.robust_estimate_subset_from.size() == 3);
                  assert(params.robust_estimate_subset_size.size() == 3);
                  Adapter::Subset<decltype(params.processed_mask)> subset (params.processed_mask, params.robust_estimate_subset_from, params.robust_estimate_subset_size);
                  LogLevelLatch log_level (0);
                  // single threaded as we loop over small VOIs. multi-threading of small VOIs is VERY slow compared to single threading!
                  for (auto i = Loop(0,3) (subset); i; ++i) {
                    kernel(subset);
                  }
                } else {
                  LogLevelLatch log_level (0);
                  ThreadedLoop (params.midway_image, 0, 3).run (kernel);
                }
              }

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

            void set_directions (const Eigen::MatrixXd& dir) {
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
