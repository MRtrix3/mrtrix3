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

#ifndef __image_registration_symmetric_metric_threadkernel_h__
#define __image_registration_symmetric_metric_threadkernel_h__

#include "math/matrix.h"
#include "image/voxel.h"
#include "image/iterator.h"
#include "image/transform.h"
#include "point.h"

namespace MR
{
  namespace Image
  {
    namespace RegistrationSymmetric
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
            ThreadKernel (const MetricType& metric, ParamType& parameters, double& overall_cost_function, Math::Vector<double>& overall_gradient) :
              metric (metric),
              params (parameters),
              cost_function (0.0),
              gradient (overall_gradient.size()),
              overall_cost_function (overall_cost_function),
              overall_gradient (overall_gradient),
              transform (params.template_image) {
                gradient.zero();
            }

            ~ThreadKernel () {
              overall_cost_function += cost_function;
              overall_gradient += gradient;
            }

            template <class U = MetricType>
            void operator() (const Image::Iterator& iter, typename is_neighbourhood_metric<U>::no = 0) {

              Point<float> midspace_point = transform.voxel2scanner (iter);
              Point<float> moving_point, template_point;
              
              // Math::Vector<double> param2moving; 
              // params.transformation.get_parameter_vector (param2moving); 

              params.transformation.transform_half (moving_point, midspace_point);

              if (params.moving_mask_interp) {
                params.moving_mask_interp->scanner (moving_point);
                if (!params.moving_mask_interp->value())
                  return;
              }

              params.transformation.transform_half_inverse (template_point, midspace_point);

              if (params.template_mask_interp) {
                params.template_mask_interp->scanner (template_point);
                if (!params.template_mask_interp->value())
                  return;
              }

              params.moving_image_interp->scanner (moving_point);
              if (!(*params.moving_image_interp))
                return;
              params.template_image_interp->scanner (template_point);
              if (!(*params.template_image_interp))
                return;
              // VAR((midspace_point-moving_point) - (template_point-midspace_point));
              cost_function += metric (params, template_point, moving_point, midspace_point, gradient);
            }

            template <class U = MetricType>
              void operator() (const Image::Iterator& iter, typename is_neighbourhood_metric<U>::yes = 0) {
                cost_function += metric (params, iter);
            }

            protected:
              MetricType metric;
              ParamType params;

              double cost_function;
              Math::Vector<double> gradient;
              double& overall_cost_function;
              Math::Vector<double>& overall_gradient;
              Image::Transform transform;
        };
      }
    }
  }
}

#endif
