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
      }
      //! \endcond

      template <class MetricType, class ParamType>
      class ThreadKernel {
        public:
          ThreadKernel (const MetricType& metric, const ParamType& parameters, default_type& overall_cost_function, Eigen::VectorXd& overall_gradient, ssize_t* overall_cnt = nullptr) :
            metric (metric),
            params (parameters),
            cost_function (0.0),
            cnt (0),
            gradient (overall_gradient.size()),
            overall_cost_function (overall_cost_function),
            overall_gradient (overall_gradient),
            overall_cnt (overall_cnt),
            transform (params.midway_image) { gradient.setZero(); }

          ~ThreadKernel () {
            overall_cost_function += cost_function;
            overall_gradient += gradient;
            if (overall_cnt) {
              (*overall_cnt) += cnt;
            }
          }

          template <class U = MetricType>
          void operator() (const Iterator& iter, typename is_neighbourhood_metric<U>::no = 0) {
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

#ifdef REGISTRATION_GRADIENT_DESCENT_DEBUG
            Eigen::Vector3 also_im1_point;
            params.transformation.transform (also_im1_point, im2_point);
            if (!also_im1_point.isApprox(im1_point)){
              VEC(im1_point.transpose());
              VEC(also_im1_point.transpose());
              VEC(im2_point.transpose());
              VEC((also_im1_point - im1_point).transpose());
              throw Exception ("this is not right");
            }
#endif
            ++cnt;
            cost_function += metric (params, im1_point, im2_point, midway_point, gradient);
          }

          template <class U = MetricType>
            void operator() (const Iterator& iter, typename is_neighbourhood_metric<U>::yes = 0, typename use_processed_image<U>::no = 0) {
              throw Exception ("neighbourhood metric without precompute method not implemented");
            }

          template <class U = MetricType>
            void operator() (const Iterator& iter, typename is_neighbourhood_metric<U>::yes = 0, typename use_processed_image<U>::yes = 0) {
              assert(params.processed_image.valid());

              Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));

              if (params.processed_mask.valid()){
                // assign_pos_of(iter).to(params.processed_mask);
                params.processed_mask.index(0) = iter.index(0);
                params.processed_mask.index(1) = iter.index(1);
                params.processed_mask.index(2) = iter.index(2);
                if (!params.processed_mask.value())
                  return;
              }
              // Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));
              // if (params.processed_mask_interp){
                // assign_pos_of(iter).to(*params.processed_mask_interp);
                // std::cerr << iter << ": mask1" << *params.processed_mask_interp << std::endl;
                // params.processed_mask_interp->scanner(midway_point);
                // params.processed_mask_interp->index(0) = voxel_pos[0];
                // params.processed_mask_interp->index(1) = voxel_pos[1];
                // params.processed_mask_interp->index(2) = voxel_pos[2];
                // if (!params.processed_mask_interp->value())
                  // return;
              // }

              Eigen::Vector3 midway_point, im1_point, im2_point;
              midway_point = transform.voxel2scanner * voxel_pos;
              params.transformation.transform_half (im1_point, midway_point);
              params.transformation.transform_half_inverse (im2_point, midway_point);

              ++cnt;
              cost_function += metric (params, iter, im1_point, im2_point, midway_point, gradient);
            }

          protected:
            MetricType metric;
            ParamType params;

            default_type cost_function;
            ssize_t cnt;
            Eigen::VectorXd gradient;
            default_type& overall_cost_function;
            Eigen::VectorXd& overall_gradient;
            ssize_t* overall_cnt;
            Transform transform;
      };
    }
  }
}

#endif
