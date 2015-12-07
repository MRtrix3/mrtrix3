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

#ifndef __registration_metric_synthreadkernel_h__
#define __registration_metric_synthreadkernel_h__

#include "image.h"
#include "algo/iterator.h"
#include "transform.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {


      template <class MetricType>
      class SynThreadKernel {
        public:
          SynThreadKernel (const MetricType& metric,
                           default_type& overall_cost_function) :
                             metric (metric),
                             overall_cost_function (overall_cost_function),
                             cost_function (0.0) {}

          ~ThreadKernel () {
            overall_cost_function += cost_function;
          }

          void set_im1_mask (const Image<float>& mask) {
            im1_mask = mask;
          }

          void set_im2_mask (const Image<float>& mask) {
            im2_mask = mask;
          }

          template <class Im1ImageType, class Im2ImageType, class Im1UpdateType, class Im2UpdateType>
          void operator() (const Im1ImageType& im1_image, const Im2ImageType& im2_image, Im1UpdateType& im1_update, Im2UpdateType& im2_update) {

            float im1_mask_value = 1.0;
            if (im1_mask.valid()) {
              assign_pos_of (im1_image).to (im1_mask);
              im1_mask_value = im1_mask.value();
              if (im1_mask_value < 0.1)
                return;
            }

            float im2_mask_value = 1.0;
            if (im2_mask.valid()) {
              assign_pos_of (im2_image).to (im2_mask);
              im2_mask_value = im2_mask.value();
              if (im2_mask_value < 0.1)
                return;
            }

            Eigen::Matrix<typename Im1UpdateType::ValueType, 1, 3> im1_grad;
            Eigen::Matrix<typename Im1UpdateType::ValueType, 1, 3> im2_grad;
            cost_function += metric (im1_image, im2_image, im1_grad, im2_grad);

            im1_update.row(3) = im1_mask_value * im1_grad;
            im2_update.row(3) = im2_mask_value * im1_grad;
          }



          protected:
            MetricType metric;
            default_type& overall_cost_function;
            default_type cost_function;
            Image<float> im1_mask;
            Image<float> im2_mask;
      };
    }
  }
}

#endif
