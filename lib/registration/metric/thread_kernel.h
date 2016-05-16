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

#ifndef __registration_metric_threadkernel_h__
#define __registration_metric_threadkernel_h__

#include "image.h"
#include "algo/iterator.h"
#include "transform.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      //! \cond skip
      namespace {
        template<class T>
        struct Void {
          typedef void type;
        };

        template <class MetricType, typename U = void>
        struct is_neighbourhood_metric {
          typedef int no;
        };

        template <class MetricType>
        struct is_neighbourhood_metric<MetricType, typename Void<typename MetricType::is_neighbourhood>::type> {
          typedef int yes;
        };

        template <class MetricType, typename U = void>
        struct use_processed_image {
          typedef int no;
        };

        template <class MetricType>
        struct use_processed_image<MetricType, typename Void<typename MetricType::requires_precompute>::type> {
          typedef int yes;
        };

        template <class MetricType, typename U = void>
        struct cost_is_vector {
          typedef int no;
        };

        template <class MetricType>
        struct cost_is_vector<MetricType, typename Void<typename MetricType::is_vector_type>::type> {
          typedef int yes;
        };
      }
      //! \endcond

      template <class MetricType, class ParamType>
      class ThreadKernel {
        public:
          ThreadKernel (
              const MetricType& metric,
              const ParamType& parameters,
              Eigen::VectorXd& overall_cost_function,
              Eigen::VectorXd& overall_gradient,
              ssize_t* overall_cnt = nullptr):
            metric (metric),
            params (parameters),
            cost_function (overall_cost_function.size()),
            cnt (0),
            gradient (overall_gradient.size()),
            overall_cost_function (overall_cost_function),
            overall_gradient (overall_gradient),
            overall_cnt (overall_cnt),
            transform (params.midway_image) {
              gradient.setZero();
              cost_function.setZero();
            }

          ~ThreadKernel () {
            overall_cost_function += cost_function;
            overall_gradient += gradient;
            if (overall_cnt)
              (*overall_cnt) += cnt;
          }

          template <class U = MetricType>
          void operator() (const Iterator& iter,
              typename is_neighbourhood_metric<U>::no = 0,
              typename use_processed_image<U>::no = 0,
              typename cost_is_vector<U>::no = 0) {

            Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));

            Eigen::Vector3 midway_point = transform.voxel2scanner * voxel_pos;

            Eigen::Vector3 im2_point;
            params.transformation.transform_half_inverse (im2_point, midway_point);
            if (params.im2_mask_interp) {
              params.im2_mask_interp->scanner (im2_point);
              if (params.im2_mask_interp->value() < 0.5)
                return;
            }

            Eigen::Vector3 im1_point;
            params.transformation.transform_half (im1_point, midway_point);
            if (params.im1_mask_interp) {
              params.im1_mask_interp->scanner (im1_point);
              if (params.im1_mask_interp->value() < 0.5)
                return;
            }

            params.im1_image_interp->scanner (im1_point);
            if (!(*params.im1_image_interp))
              return;

            params.im2_image_interp->scanner (im2_point);
            if (!(*params.im2_image_interp))
              return;

            ++cnt;
            cost_function(0) += metric (params, im1_point, im2_point, midway_point, gradient);
          }

          template <class U = MetricType>
          void operator() (const Iterator& iter,
              typename is_neighbourhood_metric<U>::no = 0,
              typename use_processed_image<U>::no = 0,
              typename cost_is_vector<U>::yes = 0) {

            Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));

            Eigen::Vector3 midway_point = transform.voxel2scanner * voxel_pos;

            Eigen::Vector3 im2_point;
            params.transformation.transform_half_inverse (im2_point, midway_point);
            if (params.im2_mask_interp) {
              params.im2_mask_interp->scanner (im2_point);
              if (params.im2_mask_interp->value() < 0.5)
                return;
            }

            Eigen::Vector3 im1_point;
            params.transformation.transform_half (im1_point, midway_point);
            if (params.im1_mask_interp) {
              params.im1_mask_interp->scanner (im1_point);
              if (params.im1_mask_interp->value() < 0.5)
                return;
            }

            params.im1_image_interp->scanner (im1_point);
            if (!(*params.im1_image_interp))
              return;

            params.im2_image_interp->scanner (im2_point);
            if (!(*params.im2_image_interp))
              return;

            ++cnt;
            cost_function.noalias() = cost_function + metric (params, im1_point, im2_point, midway_point, gradient);
          }

          template <class U = MetricType>
          void operator() (const Iterator& iter,
              typename is_neighbourhood_metric<U>::no = 0,
              typename use_processed_image<U>::yes = 0,
              typename cost_is_vector<U>::no = 0) {
            assert (params.processed_image.valid());

            if (params.processed_mask.valid()) {
              assign_pos_of (iter, 0, 3).to (params.processed_mask);
              if (!params.processed_mask.value())
                return;
            }
            ++cnt;
            cost_function(0) += metric (params, iter, gradient);
          }

          template <class U = MetricType>
            void operator() (const Iterator& iter,
                typename is_neighbourhood_metric<U>::yes = 0,
                typename use_processed_image<U>::yes = 0,
                typename cost_is_vector<U>::no = 0) {
              assert(params.processed_image.valid());

              Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));

              if (params.processed_mask.valid()){
                assign_pos_of (iter, 0, 3).to (params.processed_mask);
                assert(params.processed_mask.index(0) == iter.index(0));
                assert(params.processed_mask.index(1) == iter.index(1));
                assert(params.processed_mask.index(2) == iter.index(2));
                if (!params.processed_mask.value())
                  return;
              }

              ++cnt;
              cost_function(0) += metric (params, iter, gradient);
            }

          protected:
            MetricType metric;
            ParamType params;

            Eigen::VectorXd cost_function;
            ssize_t cnt;
            Eigen::VectorXd gradient;
            Eigen::VectorXd& overall_cost_function;
            Eigen::VectorXd& overall_gradient;
            ssize_t* overall_cnt;
            MR::Transform transform;
      };
    }
  }
}

#endif
