/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __registration_metric_difference_robust_h__
#define __registration_metric_difference_robust_h__

#include "math/math.h"
#include "registration/metric/robust_estimators.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template<class Estimator = L2>
        class DifferenceRobust { MEMALIGN(DifferenceRobust<Estimator>)
          public:
            DifferenceRobust (Estimator est) : estimator(est) {}

            template <class Params>
              default_type operator() (Params& params,
                                       const Eigen::Vector3 im1_point,
                                       const Eigen::Vector3 im2_point,
                                       const Eigen::Vector3 midway_point,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

                typename Params::Im1ValueType im1_value;
                typename Params::Im2ValueType im2_value;
                Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;
                Eigen::Matrix<typename Params::Im2ValueType, 1, 3> im2_grad;

                params.im1_image_interp->value_and_gradient_wrt_scanner (im1_value, im1_grad);
                if (std::isnan (default_type (im1_value)))
                  return 0.0;
                params.im2_image_interp->value_and_gradient_wrt_scanner (im2_value, im2_grad);
                if (std::isnan (default_type (im2_value)))
                  return 0.0;

                default_type residual, grad;
                estimator((default_type) im1_value - (default_type) im2_value, residual, grad);
                const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);
                const  Eigen::Vector3d g = grad * (im1_grad + im2_grad);
                gradient.segment<4>(0) += g(0) * jacobian_vec;
                gradient.segment<4>(4) += g(1) * jacobian_vec;
                gradient.segment<4>(8) += g(2) * jacobian_vec;

                return residual;
            }

            Estimator estimator;
        };

      template<class Im1Type, class Im2Type, class Estimator = L2>
        class DifferenceRobust4D { MEMALIGN(DifferenceRobust4D<Im1Type,Im2Type,Estimator>)
          public:
            DifferenceRobust4D (const Im1Type& im1, const Im2Type& im2, const Estimator& est) :
              volumes(im1.size(3)),
              estimator(est) {
              im1_grad.resize(volumes, 3);
              im2_grad.resize(volumes, 3);
              im1_values.resize(volumes, 1);
              im2_values.resize(volumes, 1);
              diff_values.resize(volumes, 1);
            };

          /** requires_initialisation:
          type_trait to distinguish metric types that require a call to init before the operator() is called */
          typedef int requires_initialisation;

          void init (const Im1Type& im1, const Im2Type& im2) {
            assert (im1.ndim() == 4);
            assert (im2.ndim() == 4);
            assert(im1.size(3) == im2.size(3));
            if (volumes != im1.size(3)) {
                volumes = im1.size(3);
                im1_grad.resize(volumes, 3);
                im2_grad.resize(volumes, 3);
                im1_values.resize(volumes, 1);
                im2_values.resize(volumes, 1);
                diff_values.resize(volumes, 1);
              }
          }

          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
              if (im1_values.hasNaN())
                return 0.0;

              params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);
              if (im2_values.hasNaN())
                return 0.0;

              assert (volumes == im1_grad.rows() && "metric.init not called after image has been cropped?");
              assert (volumes == im2_grad.rows() && "metric.init not called after image has been cropped?");

              const Eigen::Matrix<default_type, 4, 1> jacobian_vec (params.transformation.get_jacobian_vector_wrt_params (midway_point));
              diff_values = im1_values - im2_values;

              Eigen::Matrix<default_type, Eigen::Dynamic, 1> residuals, grads;
              estimator (diff_values.template cast<default_type>(), residuals, grads);

              Eigen::Matrix<default_type, 1, 3> g;
              for (ssize_t i = 0; i < volumes; ++i) {
                g = grads[i] * (im1_grad.row(i) + im2_grad.row(i));
                gradient.segment<4>(0) += g(0) * jacobian_vec;
                gradient.segment<4>(4) += g(1) * jacobian_vec;
                gradient.segment<4>(8) += g(2) * jacobian_vec;
              }

              return residuals.sum() / (default_type)volumes;
            }

          private:
            ssize_t volumes;
            Estimator estimator;
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 3> im1_grad;
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 3> im2_grad;
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> im1_values, diff_values;
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 1> im2_values;
        };
    }
  }
}
#endif
