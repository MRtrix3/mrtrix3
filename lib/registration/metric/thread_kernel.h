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
      }
      //! \endcond


      template <class MetricType, class ParamType>
      class ThreadKernel {
        public:
          ThreadKernel (const MetricType& metric, ParamType& parameters, default_type& overall_cost_function, Eigen::VectorXd& overall_gradient) :
            metric (metric),
            params (parameters),
            cost_function (0.0),
            gradient (overall_gradient.size()),
            overall_cost_function (overall_cost_function),
            overall_gradient (overall_gradient),
            #ifdef NONSYMREGISTRATION
              transform (params.template_image) {
            #else
              transform (params.midway_image) {
            #endif
              gradient.setZero();
          }

          ~ThreadKernel () {
            overall_cost_function += cost_function;
            overall_gradient += gradient;
          }

          #ifdef NONSYMREGISTRATION
            template <class U = MetricType>
            void operator() (const Iterator& iter, typename is_neighbourhood_metric<U>::no = 0) {
              Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));
              Eigen::Vector3 template_point = transform.voxel2scanner * voxel_pos;
              if (params.template_mask_interp) {
                params.template_mask_interp->scanner (template_point);
                if (!params.template_mask_interp->value())
                  return;
              }

              Eigen::Vector3 moving_point;

              params.transformation.transform (moving_point, template_point);
              if (params.moving_mask_interp) {
                params.moving_mask_interp->scanner (moving_point);
                if (!params.moving_mask_interp->value())
                  return;
              }
              assign_pos_of (iter).to (params.template_image);
              params.moving_image_interp->scanner (moving_point);
              if (!(*params.moving_image_interp))
                return;
              cost_function += metric (params, template_point, gradient);
            }
          #else
            template <class U = MetricType>
            void operator() (const Iterator& iter, typename is_neighbourhood_metric<U>::no = 0) {
              Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));

              Eigen::Vector3 midway_point = transform.voxel2scanner * voxel_pos;


              Eigen::Vector3 template_point;
              params.transformation.transform_half_inverse (template_point, midway_point);
              if (params.template_mask_interp) {
                params.template_mask_interp->scanner (template_point);
                if (!params.template_mask_interp->value())
                  return;
              }

              Eigen::Vector3 moving_point;
              params.transformation.transform_half (moving_point, midway_point);
              if (params.moving_mask_interp) {
                params.moving_mask_interp->scanner (moving_point);
                if (!params.moving_mask_interp->value())
                  return;
              }
              
              params.moving_image_interp->scanner (moving_point);
              if (!(*params.moving_image_interp))
                return;
              
              params.template_image_interp->scanner (template_point);
              if (!(*params.template_image_interp))
                return;

              cost_function += metric (params, template_point, moving_point, midway_point, gradient);
            }
          #endif

          template <class U = MetricType>
            void operator() (const Iterator& iter, typename is_neighbourhood_metric<U>::yes = 0) {
              cost_function += metric (params, iter);
          }

          protected:
            MetricType metric;
            ParamType params;

            default_type cost_function;
            Eigen::VectorXd gradient;
            default_type& overall_cost_function;
            Eigen::VectorXd& overall_gradient;
            Transform transform;
      };
    }
  }
}

#endif
