/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 2013

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

#ifndef __registration_transform_reorient_h__
#define __registration_transform_reorient_h__

#include "algo/threaded_loop.h"
#include "math/SH.h"
#include "math/least_squares.h"
#include "interp/cubic.h"

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
      class LinearKernel {

        public:
          LinearKernel (const ssize_t n_SH,
                        const transform_type& linear_transform,
                        const Eigen::MatrixXd& directions,
                        const bool modulate)
          {
            Eigen::MatrixXd transformed_directions = linear_transform.linear().inverse() * directions;

            if (modulate) {
              Eigen::VectorXd modulation_factors = transformed_directions.colwise().norm() / linear_transform.linear().inverse().determinant();
              transformed_directions.colwise().normalize();
              transform = (aPSF_weights_to_FOD_transform (n_SH, transformed_directions) * modulation_factors.asDiagonal()
                           * Math::pinv(aPSF_weights_to_FOD_transform (n_SH, directions))).cast <typename FODImageType::value_type> ();
            } else {
              transformed_directions.colwise().normalize();
              transform = (aPSF_weights_to_FOD_transform (n_SH, transformed_directions)
                           * Math::pinv(aPSF_weights_to_FOD_transform (n_SH, directions))).cast <typename FODImageType::value_type> ();
            }
          }

          void operator() (FODImageType& image)
          {
            image.index(3) = 0;
            if (image.value() > 0.0)  // only reorient voxels that contain a FOD
              image.row(3) = transform * image.row(3); // Eigen automatically takes care of the temporary
          }

        protected:
          Eigen::Matrix<typename FODImageType::value_type, Eigen::Dynamic, Eigen::Dynamic> transform;
      };




      template <class FODImageType>
      void reorient (FODImageType& fod_image,
                     const transform_type& transform,
                     const Eigen::MatrixXd& directions,
                     bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        ThreadedLoop (fod_image, 0, 3)
            .run (LinearKernel<FODImageType>(fod_image.size(3), transform, directions, modulate), fod_image);
      }



      template <class FODImageType>
      void reorient (const std::string progress_message,
                     FODImageType& fod_image,
                     const transform_type& transform,
                     const Eigen::MatrixXd& directions,
                     bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        ThreadedLoop (progress_message, fod_image, 0, 3)
            .run (LinearKernel<FODImageType>(fod_image.size(3), transform, directions, modulate), fod_image);
      }




      template <class FODImageType, class WarpImageType>
      class NonLinearKernel {

        public:
          NonLinearKernel (const ssize_t n_SH, WarpImageType& warp, const Eigen::MatrixXd& directions, const bool modulate) :
                             n_SH (n_SH),
                             warp_interp (warp),
                             directions (directions),
                             modulate (modulate),
                             FOD_to_aPSF_transform (Math::pinv(aPSF_weights_to_FOD_transform (n_SH, directions))) {}


          void operator() (FODImageType& image) {
            image.index(3) = 0;
            if (image.value() > 0) {  // only reorient voxels that contain a FOD
              Eigen::Vector3d vox;
              vox[0] = static_cast<default_type> (image.index(0));
              vox[1] = static_cast<default_type> (image.index(1));
              vox[2] = static_cast<default_type> (image.index(2));
              warp_interp.voxel(vox);
              Eigen::MatrixXd jacobian = warp_interp.gradient_row_wrt_scanner().inverse();
              Eigen::MatrixXd transformed_directions = jacobian * directions;

              if (modulate) {
                Eigen::MatrixXd modulation_factors = transformed_directions.colwise().norm() / jacobian.determinant();
                transformed_directions.colwise().normalize();
                transform = (aPSF_weights_to_FOD_transform (n_SH, transformed_directions) * modulation_factors.asDiagonal()
                             * FOD_to_aPSF_transform);
              } else {
                transformed_directions.colwise().normalize();
                transform = aPSF_weights_to_FOD_transform (n_SH, transformed_directions) * FOD_to_aPSF_transform;
              }

              image.row(3) = transform.cast<typename WarpImageType::value_type>() * image.row(3);
            }
          }
          protected:
            const ssize_t n_SH;
            Interp::SplineInterp<WarpImageType,
                                 Math::UniformBSpline<typename WarpImageType::value_type>,
                                 Math::SplineProcessingType::Derivative> warp_interp;
            const Eigen::MatrixXd& directions;
            const bool modulate;
            const Eigen::MatrixXd FOD_to_aPSF_transform;
            Eigen::MatrixXd transform;
      };


      template <class FODImageType, class WarpImageType>
      void reorient_warp (const std::string progress_message,
                          FODImageType& fod_image,
                          WarpImageType& warp,
                          const Eigen::MatrixXd& directions,
                          const bool modulate = false)
      {
        assert (directions.cols() > directions.rows());
        ThreadedLoop (progress_message, fod_image, 0, 3)
            .run (NonLinearKernel<FODImageType, WarpImageType>(fod_image.size(3), warp, directions, modulate), fod_image);
      }


    }
  }
}

#endif
