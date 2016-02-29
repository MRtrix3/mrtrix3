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

#ifndef __registration_metric_mean_squared_no_gradient_h__
#define __registration_metric_mean_squared_no_gradient_h__

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class MeanSquaredNoGradient {

        public:
          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3 im1_point,
                                     const Eigen::Vector3 im2_point,
                                     const Eigen::Vector3 midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              typename Params::Im1ValueType im1_value;
              typename Params::Im2ValueType im2_value;

              im1_value = params.im1_image_interp->value ();
              if (isnan (default_type (im1_value)))
                return 0.0;
              im2_value = params.im2_image_interp->value ();
              if (isnan (default_type (im2_value)))
                return 0.0;

              default_type diff = (default_type) im1_value - (default_type) im2_value;

              return diff * diff;
          }

      };
    }
  }
}
#endif
