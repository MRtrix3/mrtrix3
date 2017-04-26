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


#ifndef __registration_transform_reorient_h__
#define __registration_transform_reorient_h__

#include "algo/threaded_loop.h"
#include "math/SH.h"
#include "math/least_squares.h"
#include "adapter/jacobian.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      FORCE_INLINE Eigen::MatrixXd aPSF_weights_to_FOD_transform (const int num_SH, const Eigen::MatrixXd& directions)
      {
        Eigen::MatrixXd aPSF_matrix (num_SH, directions.cols());
        Math::SH::aPSF<default_type> aPSF_generator (Math::SH::LforN (num_SH));
        Eigen::Matrix<default_type, Eigen::Dynamic,1> aPSF;
        for (ssize_t i = 0; i < directions.cols(); ++i) {
          aPSF_matrix.col(i) = aPSF_generator (aPSF, directions.col(i));
        }
        return aPSF_matrix;
      }



      template <class FODImageType>
      class LinearKernel { MEMALIGN(LinearKernel<FODImageType>)

        public:
          LinearKernel (const ssize_t n_SH,
                        const transform_type& linear_transform,
                        const Eigen::MatrixXd& directions,
                        const bool modulate) : fod (n_SH)
          {
            Eigen::MatrixXd transformed_directions = linear_transform.linear().inverse() * directions;

            if (modulate) {
              Eigen::VectorXd modulation_factors = transformed_directions.colwise().norm() / linear_transform.linear().inverse().determinant();
              transformed_directions.colwise().normalize();
              transform.noalias() = aPSF_weights_to_FOD_transform (n_SH, transformed_directions) * modulation_factors.asDiagonal()
                                  * Math::pinv (aPSF_weights_to_FOD_transform (n_SH, directions));
            } else {
              transformed_directions.colwise().normalize();
              transform.noalias() = aPSF_weights_to_FOD_transform (n_SH, transformed_directions)
                                  * Math::pinv (aPSF_weights_to_FOD_transform (n_SH, directions));
            }
          }

          void operator() (FODImageType& in, FODImageType& out)
          {
            in.index(3) = 0;
            if (in.value() > 0.0) { // only reorient voxels that contain a FOD
              fod = in.row(3);
              fod = transform * fod;
              out.row(3) = fod;
            }
          }

        protected:
          Eigen::MatrixXd transform;
          Eigen::VectorXd fod; 
      };


      /*
      * reorient all FODs in an image with a linear transform.
      * Note the input image can be the same as the output image
      */
      template <class FODImageType>
      void reorient (FODImageType& input_fod_image,
                     FODImageType& output_fod_image,
                     const transform_type& transform,
                     const Eigen::MatrixXd& directions,
                     bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        ThreadedLoop (input_fod_image, 0, 3)
            .run (LinearKernel<FODImageType>(input_fod_image.size(3), transform, directions, modulate), input_fod_image, output_fod_image);
      }


      /*
      * reorient all FODs in an image with a linear transform.
      * Note the input image can be the same as the output image
      */
      template <class FODImageType>
      void reorient (const std::string progress_message,
                     FODImageType& input_fod_image,
                     FODImageType& output_fod_image,
                     const transform_type& transform,
                     const Eigen::MatrixXd& directions,
                     bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        ThreadedLoop (progress_message, input_fod_image, 0, 3)
            .run (LinearKernel<FODImageType>(input_fod_image.size(3), transform, directions, modulate), input_fod_image, output_fod_image);
      }




      template <class FODImageType>
      class NonLinearKernel { MEMALIGN(NonLinearKernel<FODImageType>)

        public:
          NonLinearKernel (const ssize_t n_SH, Image<default_type>& warp, const Eigen::MatrixXd& directions, const bool modulate) :
                           n_SH (n_SH),
                           jacobian_adapter (warp),
                           directions (directions),
                           modulate (modulate),
                           FOD_to_aPSF_transform (Math::pinv (aPSF_weights_to_FOD_transform (n_SH, directions))),
                           fod (n_SH) {}


          void operator() (FODImageType& image) {
            image.index(3) = 0;
            if (image.value() > 0) {  // only reorient voxels that contain a FOD
              for (size_t dim = 0; dim < 3; ++dim)
                jacobian_adapter.index(dim) = image.index(dim);
              Eigen::MatrixXd jacobian = jacobian_adapter.value().inverse().template cast<default_type>();
              Eigen::MatrixXd transformed_directions = jacobian * directions;

              if (modulate) {
                Eigen::MatrixXd modulation_factors = transformed_directions.colwise().norm() / jacobian.determinant();
                transformed_directions.colwise().normalize();

                Eigen::MatrixXd temp = aPSF_weights_to_FOD_transform (n_SH, transformed_directions);
                for (ssize_t i = 0; i < temp.cols(); ++i)
                  temp.col(i) = temp.col(i) * modulation_factors(0,i);

                transform.noalias() = temp * FOD_to_aPSF_transform;
              } else {
                transformed_directions.colwise().normalize();
                transform.noalias() = aPSF_weights_to_FOD_transform (n_SH, transformed_directions) * FOD_to_aPSF_transform;
              }
              fod = image.row(3);
              fod = transform * fod;
              image.row(3) = fod;
            }
          }
          protected:
            const ssize_t n_SH;
            Adapter::Jacobian<Image<default_type> > jacobian_adapter;
            const Eigen::MatrixXd& directions;
            const bool modulate;
            const Eigen::MatrixXd FOD_to_aPSF_transform;
            Eigen::MatrixXd transform;
            Eigen::VectorXd fod;
      };


      template <class FODImageType>
      void reorient_warp (const std::string progress_message,
                          FODImageType& fod_image,
                          Image<default_type>& warp,
                          const Eigen::MatrixXd& directions,
                          const bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        check_dimensions (fod_image, warp, 0, 3);
        ThreadedLoop (progress_message, fod_image, 0, 3)
            .run (NonLinearKernel<FODImageType>(fod_image.size(3), warp, directions, modulate), fod_image);
      }

      template <class FODImageType>
      void reorient_warp (FODImageType& fod_image,
                          Image<default_type>& warp,
                          const Eigen::MatrixXd& directions,
                          const bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        check_dimensions (fod_image, warp, 0, 3);
        ThreadedLoop (fod_image, 0, 3)
            .run (NonLinearKernel<FODImageType>(fod_image.size(3), warp, directions, modulate), fod_image);
      }


    }
  }
}

#endif
