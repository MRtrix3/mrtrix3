/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by Maximilian Pietsch, 05/02/2015

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

#ifndef __image_registration_metric_cross_correlation_h__
#define __image_registration_metric_cross_correlation_h__

// #include "image/registration/metric/base.h"
// #include "point.h"
// #include "math/vector.h"
// #include <cmath>
#include "algo/loop.h"
// #include "algo/neighbourhooditerator.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      class CrossCorrelation {

          public:

            /** typedef int is_neighbourhood: type_trait to distinguish voxel-wise and neighbourhood based metric types */
            typedef int is_neighbourhood;

            typedef Eigen::Matrix< default_type, 3, Eigen::Dynamic > NeighbType1;
            typedef Eigen::Matrix< default_type, 3, Eigen::Dynamic > NeighbType2;

            template <class Params>
              default_type operator() (Params& params,
                                       // const Iterator& iter,
                                       const NeighbType1 neighbh1,
                                       const NeighbType2 neighbh2,
                                       const Eigen::Vector3 template_point,
                                       const Eigen::Vector3 moving_point,
                                       const Eigen::Vector3 midway_point,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                
                // std::cerr << neighbh1.size() /3 << std::endl;

                Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (midway_point);

                return 0.0;
            }
      };
    }
  }
}
#endif
