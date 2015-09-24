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
#include "algo/neighbourhooditerator.h"
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
              Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));

              typedef Eigen::Matrix< default_type, 3, Eigen::Dynamic > NeighbhType1;
              typedef Eigen::Matrix< default_type, 3, Eigen::Dynamic > NeighbhType2;

              NeighbhType1 neighbh1;
              NeighbhType2 neighbh2;
              
              // mat * Map<Matrix<float, 3, Dynamic> >(data,3,N).colwise().homogeneous()

              Eigen::Vector3 midway_point, moving_point, template_point;
              midway_point = transform.voxel2scanner * voxel_pos;
              params.transformation.transform_half_inverse (template_point, midway_point);
              params.transformation.transform_half (moving_point, midway_point);
              
              params.moving_image_interp->scanner (moving_point);
              if (!(*params.moving_image_interp))
                return;
              
              params.template_image_interp->scanner (template_point);
              if (!(*params.template_image_interp))
                return;

              if (params.moving_mask_interp) {
                params.moving_mask_interp->scanner (moving_point);
                if (!params.moving_mask_interp->value())
                  return;
              }

              if (params.template_mask_interp) {
                params.template_mask_interp->scanner (template_point);
                if (!params.template_mask_interp->value())
                  return;
              }

              Eigen::Vector3 mid_point, point1, point2;
              mid_point.setZero(); // avoid compiler warning [-Wmaybe-uninitialized]
              point1.setZero();
              point2.setZero();
              auto extent = params.get_extent();
              auto niter = NeighbourhoodIterator(iter, extent);
              size_t actual_size = 0;
              size_t anticipated_size = 0;
              while(niter.loop()){
                // std::cerr << niter << std::endl;
                anticipated_size = niter.extent(0) * niter.extent(1) * niter.extent(2);
                neighbh1.resize(3,anticipated_size);
                neighbh2.resize(3,anticipated_size);

                Eigen::Vector3 vox ((default_type)niter.index(0), (default_type)niter.index(1), (default_type)niter.index(2));
                mid_point = transform.voxel2scanner * vox;
                
                params.transformation.transform_half (point1, mid_point);
                params.moving_image_interp->scanner (point1);
                if (!(*params.moving_image_interp))
                  continue;
                if (params.moving_mask_interp) {
                  params.moving_mask_interp->scanner (point1);
                  if (!params.moving_mask_interp->value())
                    continue;
                }

                params.transformation.transform_half_inverse (point2, mid_point);
                params.template_image_interp->scanner (point2);
                if (!(*params.template_image_interp))
                  continue;
                if (params.template_mask_interp) {
                  params.template_mask_interp->scanner (point2);
                  if (!params.template_mask_interp->value())
                    continue;
                }

                neighbh1.col(actual_size) = point1;
                neighbh2.col(actual_size) = point2;
                actual_size += 1;
              }

              if (actual_size == 0)
                throw Exception ("neighbourhood does not include centre");

              if (anticipated_size != actual_size){
                neighbh1.conservativeResize(neighbh1.rows(), actual_size);
                neighbh2.conservativeResize(neighbh2.rows(), actual_size);
              }

              cost_function += metric (params, neighbh1, neighbh2, template_point, moving_point, mid_point, gradient);
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
