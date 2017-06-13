/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __registration_metric_mean_squared_h__
#define __registration_metric_mean_squared_h__

#include "math/math.h"
#include "registration/metric/linear_base.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      class MeanSquared : public LinearBase { MEMALIGN(MeanSquared)
        public:
          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              assert (!this->weighted && "FIXME: set_weights not implemented for 3D metric");

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

              default_type diff = (default_type) im1_value - (default_type) im2_value;

              const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);
              const Eigen::Vector3d g = diff * (im1_grad + im2_grad);
              gradient.segment<4>(0) += g(0) * jacobian_vec;
              gradient.segment<4>(4) += g(1) * jacobian_vec;
              gradient.segment<4>(8) += g(2) * jacobian_vec;

              return diff * diff;
          }
      };

      class MeanSquaredNoGradient : public LinearBase { MEMALIGN(MeanSquaredNoGradient)
        public:
          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              assert (!this->weighted && "FIXME: set_weights not implemented for 3D metric");

              typename Params::Im1ValueType im1_value;
              typename Params::Im2ValueType im2_value;

              im1_value = params.im1_image_interp->value ();
              if (std::isnan (default_type (im1_value)))
                return 0.0;
              im2_value = params.im2_image_interp->value ();
              if (std::isnan (default_type (im2_value)))
                return 0.0;

              default_type diff = (default_type) im1_value - (default_type) im2_value;

              return diff * diff;
          }
      };

      class MeanSquaredNonSymmetric : public LinearBase { MEMALIGN(MeanSquaredNonSymmetric)
        public:
          template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              assert (!this->weighted && "FIXME: set_weights not implemented for 3D metric");

              typename Params::Im1ValueType im1_value;
              typename Params::Im2ValueType im2_value;
              Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;

              params.im1_image_interp->value_and_gradient_wrt_scanner (im1_value, im1_grad);
              if (std::isnan (default_type (im1_value)))
                return 0.0;
              im2_value = params.im2_image_interp->value ();
              if (std::isnan (default_type (im2_value)))
                return 0.0;

              default_type diff = (default_type) im1_value - (default_type) im2_value;

              const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (im2_point);
              const Eigen::Vector3d g = diff * im1_grad;
              gradient.segment<4>(0) += g(0) * jacobian_vec;
              gradient.segment<4>(4) += g(1) * jacobian_vec;
              gradient.segment<4>(8) += g(2) * jacobian_vec;

              return diff * diff;
          }
      };

      template <class Im1Type, class Im2Type>
        class MeanSquared4D : public LinearBase { MEMALIGN(MeanSquared4D<Im1Type,Im2Type>)
          public:
            template <class Params>
            default_type operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              const ssize_t volumes = params.im1_image_interp->size(3);

              if (im1_values.rows() != volumes) {
                im1_values.resize (volumes);
                im1_grad.resize (volumes, 3);
                diff_values.resize (volumes);
                im2_values.resize (volumes);
                im2_grad.resize (volumes, 3);
              }

              params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
              if (im1_values.hasNaN())
                return 0.0;

              params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);
              if (im2_values.hasNaN())
                return 0.0;

              const Eigen::Matrix<default_type, 4, 1> jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);
              diff_values = im1_values - im2_values;

              if (this->weighted)
                diff_values.array() *= this->mc_weights.array();

              for (ssize_t i = 0; i < volumes; ++i) {
                const Eigen::Vector3d g = diff_values[i] * (im1_grad.row(i) + im2_grad.row(i));
                gradient.segment<4>(0) += g(0) * jacobian_vec;
                gradient.segment<4>(4) += g(1) * jacobian_vec;
                gradient.segment<4>(8) += g(2) * jacobian_vec;
              }

              return diff_values.squaredNorm() / volumes;
            }
          private:
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> im1_values;
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 1> im2_values;
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 3> im1_grad;
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 3> im2_grad;
            Eigen::VectorXd diff_values;
        };

      template <class Im1Type, class Im2Type>
        class MeanSquaredNoGradient4D : public LinearBase { MEMALIGN(MeanSquaredNoGradient4D<Im1Type,Im2Type>)
          public:
            MeanSquaredNoGradient4D ( ) {}

            template <class Params>
              default_type operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                const ssize_t volumes = params.im1_image_interp->size(3);

                if (im1_values.rows() != volumes) {
                  im1_values.resize (volumes);
                  im2_values.resize (volumes);
                  diff_values.resize (volumes);
                }

                params.im1_image_interp->row (im1_values);
                if (im1_values.hasNaN())
                  return 0.0;

                params.im2_image_interp->row (im2_values);
                if (im2_values.hasNaN())
                  return 0.0;

                diff_values = im1_values - im2_values;
                if (this->weighted)
                  diff_values.array() *= this->mc_weights.array();

                return diff_values.squaredNorm() / volumes;
            }
          private:
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> im1_values;
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 1> im2_values;
            Eigen::VectorXd diff_values;
        };

      template <class Im1Type, class Im2Type>
        class MeanSquaredVectorNoGradient4D : public LinearBase { MEMALIGN(MeanSquaredVectorNoGradient4D<Im1Type,Im2Type>)
          public:
            MeanSquaredVectorNoGradient4D () = delete;
            MeanSquaredVectorNoGradient4D ( const Im1Type im1, const Im2Type im2 ):
              volumes ( (im1.ndim() == 3) ? 1 : im1.size(3) ) {
                assert (im1.ndim() == 4);
                assert (im1.ndim() == im2.ndim());
                assert (im1.size(3) == im2.size(3));
                diff_values.resize (volumes);
                im1_values.resize (volumes);
                im2_values.resize (volumes);
              }

            //type_trait to indicate return type of operator()
            using is_vector_type = int;

            template <class Params>
              Eigen::Matrix<default_type, Eigen::Dynamic, 1> operator() (Params& params,
                                     const Eigen::Vector3& im1_point,
                                     const Eigen::Vector3& im2_point,
                                     const Eigen::Vector3& midway_point,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              assert (volumes == params.im1_image_interp->size(3));

              im1_values = params.im1_image_interp->row (3);
              if (im1_values.hasNaN()) {
                diff_values.setZero();
                return diff_values;
              }

              im2_values = params.im2_image_interp->row (3);
              if (im2_values.hasNaN()) {
                diff_values.setZero();
                return diff_values;
              }

              diff_values = im1_values - im2_values;
              if (this->weighted)
                diff_values.array() *= this->mc_weights.array();

              return diff_values.array().square();
            }
          private:
            size_t volumes;
            Eigen::Matrix<typename Im1Type::value_type, Eigen::Dynamic, 1> im1_values;
            Eigen::Matrix<typename Im2Type::value_type, Eigen::Dynamic, 1> im2_values;
            Eigen::VectorXd diff_values;
        };
    }
  }
}
#endif
