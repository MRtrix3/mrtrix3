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

#ifndef __registration_metric_mututual_information_h__
#define __registration_metric_mututual_information_h__

#include "point.h"
#include <cmath>

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class MutualInformation : public Base {
        public:

          typedef double value_type;

          template <class Params>
            double operator() (Params& params,
                               Eigen::Vector3 im1_point,
                               Eigen::Vector3 im2_point,
                               Eigen::Vector3& gradient) {

              params.transformation.get_jacobian_wrt_params (im2_point, jacobian);

              value_type diff = params.im1_image_interp.value() - params.target_image.value();

              for (ssize_t par = 0; par < gradient.size(); par++) {
                value_type sum = 0.0;
                for( size_t dim = 0; dim < 3; dim++) {
                  sum += 2.0 * diff * jacobian(dim, par) * im1_grad[dim];
                }
                gradient[par] += sum;
              }
              return diff * diff;
          }

        protected:

          value_type evaluate_cubic_bspline_kernel (value_type val) {
            const value_type abs_val = std::abs(val);
            if (abs_val  < 1.0) {
              const value_type sqr_val = abs_val * abs_val;
              return (4.0 - 6.0 * sqr_val + 3.0 * sqr_val * abs_val) / 6.0;
            } else if (abs_val < 2.0) {
              const value_type sqr_val = abs_val * abs_val;
              return (8.0 - 12.0 * abs_val + 6.0 * sqr_val - sqr_val * abs_val) / 6.0;
            } else {
              return 0.0;
            }
          }

          size_t m_NumberOfHistogramBins;
          double  m_MovingImageNormalizedMin;
          double  m_FixedImageNormalizedMin;
          double  m_FixedImageTrueMin;
          double  m_FixedImageTrueMax;
          double  m_MovingImageTrueMin;
          double  m_MovingImageTrueMax;
          double  m_FixedImageBinSize;
          double  m_MovingImageBinSize;

      };
    }
  }
}
#endif
