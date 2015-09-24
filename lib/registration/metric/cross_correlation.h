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
                                       const Eigen::Vector3 im1_point,
                                       const Eigen::Vector3 im2_point,
                                       const Eigen::Vector3 midway_point,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                
                Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (midway_point);

                assert (neighbh1.cols() == neighbh2.cols());
                const size_t elements = neighbh1.cols();

                Eigen::Matrix< typename Params::MovingValueType, Eigen::Dynamic, 1 > I;
                Eigen::Matrix< typename Params::TemplateValueType, Eigen::Dynamic, 1 > J;
                I.resize(elements,1);
                J.resize(elements,1);

                typename Params::MovingValueType im1_value;
                typename Params::TemplateValueType im2_value;
                Eigen::Matrix<typename Params::MovingValueType, 1, 3> im1_grad;
                Eigen::Matrix<typename Params::TemplateValueType, 1, 3> im2_grad;
                for (auto i = 0; i < elements; ++i) { // TODO imX_grad unused
                  params.im1_processed_interp->scanner (neighbh1.col(i));
                  params.im1_processed_interp->value_and_gradient (I[i], im1_grad);
                  params.im2_processed_interp->scanner (neighbh2.col(i));
                  params.im2_processed_interp->value_and_gradient (J[i], im2_grad);
                  if (isnan (default_type (I[i])))
                    return 0.0;
                  if (isnan (default_type (J[i])))
                    return 0.0;
                }

                default_type A, B, C;
                // VAR(J);
                A = I.dot(J);
                B = I.dot(I);
                C = J.dot(J);

                // VAR(I.transpose());
                // std::cerr << neighbh1 << std::endl;
                // VAR(J.transpose());
                // std::cerr << neighbh2 << std::endl;
                // VAR(A);
                // VAR(B);
                // VAR(C);

                params.im1_processed_interp->scanner (im1_point);
                params.im1_processed_interp->value_and_gradient (im1_value, im1_grad);
                params.im2_processed_interp->scanner (im2_point);
                params.im2_processed_interp->value_and_gradient (im2_value, im2_grad);

                // TODO gradient
                // for (size_t par = 0; par < gradient.size(); par++) {
                  // default_type sum = 0.0;
                  // for ( size_t dim = 0; dim < 3; dim++){
                    // sum += A/(B*C) * jacobian (dim, par) * (im2_value - A/B * im1_value) * im1_grad[dim];
                    // sum += A/(B*C) * jacobian (dim, par) * (im1_value - A/C * im2_value) * im2_grad[dim];
                  // }
                  // gradient[par] += sum;
                // }
                // jacobian.determinant()

                return (default_type) A*A/(B*C);
            }
      };
    }
  }
}
#endif
