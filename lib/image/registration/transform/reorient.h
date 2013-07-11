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

#ifndef __image_registration_transform_reorient_h__
#define __image_registration_transform_reorient_h__

#include "image/threaded_loop.h"
#include "image/adapter/gradient1D.h"
#include "math/SH.h"
#include "image/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Transform
      {




//        template <class FODVoxelType>
//        class Base {
//
//          public:
//            Base (FODVoxelType& fod_voxel,
//                  const Math::Matrix<float>& directions_transposed,
//                  const Math::Matrix<float>& fod_to_aPSF_weights_transform) :
//                  fod_voxel (fod_voxel),
//                  fod_to_aPSF_weights_transform (fod_to_aPSF_weights_transform),
//                  aPSF_generator (Math::SH::LforN(fod_voxel.dim(3))) {
//                    directions = Math::transpose(directions_transposed);}
//
//          protected:
//            void reorient_FOD () {
//              Math::Vector<float> fod (fod_voxel.address(), fod_voxel.dim(3));
//              Math::mult (weights, fod_to_aPSF_weights_transform, fod);
//              Math::mult (transformed_directions, transform, directions);
//              fod.zero();
//              // for each direction, compute an apodised PSF, add it to the FOD with the weight
//              Math::Vector<float> aPSF;
//              for (int i = 0; i < transformed_directions.columns(); ++i) {
//                Point<float> dir (transformed_directions (0, i), transformed_directions (1, i), transformed_directions (2, i));
//                aPSF_generator (aPSF, dir);
//                aPSF *= weights[i];
//                fod += aPSF;
//              }
//            }
//
////            void reorient_FOD () {
////              Math::Vector<float> fod (fod_voxel.address(), fod_voxel.dim(3));
////              Math::mult (weights, fod_to_aPSF_weights_transform, fod);
////              Math::mult (transformed_directions, transform, directions);
////              fod.zero();
////              // for each direction, compute an apodised PSF, add it to the FOD with the weight
////              Math::Vector<float> aPSF;
////              for (int i = 0; i < transformed_directions.columns(); ++i) {
////                Point<float> dir (transformed_directions (0, i), transformed_directions (1, i), transformed_directions (2, i));
////                aPSF_generator (aPSF, dir);
////                aPSF *= weights[i];
////                fod += aPSF;
////              }
////            }
//
//            FODVoxelType fod_voxel;
//            Math::Matrix<float> fod_to_aPSF_weights_transform;
//            Math::Matrix<float> transform;
//
//            Math::Matrix<float> transformed_directions;
//            Math::Vector<float> weights;
//            Math::Matrix<float> directions;
//        };




        template <class FODVoxelType>
        class LinearReorientKernel {

        public:
          LinearReorientKernel (FODVoxelType& fod_image_in,
                                FODVoxelType& fod_image_out,
                                const Math::Matrix<float>& directions_transposed,
                                const Math::Matrix<float>& transform) :
                                  fod_voxel_in (fod_image_in),
                                  fod_voxel_out (fod_image_out),
                                  fod_in (fod_image_in.dim(3)),
                                  fod_out (fod_image_out.dim(3)) {

            Image::check_dimensions(fod_image_in, fod_image_out);

            Math::Matrix<float> forward_transform;
            Math::LU::inv (forward_transform, transform.sub(0,3,0,3));

            Math::Matrix<float> transformed_directions;
            Math::mult (transformed_directions, forward_transform, Math::transpose (directions_transposed));

            Math::Matrix<float> fod_to_aPSF_weights_transform;
            precompute_FOD_to_aPSF_weights_transform (fod_image_in.dim(3), directions_transposed, fod_to_aPSF_weights_transform);

            Math::SH::aPSF<float> aPSF_generator (Math::SH::LforN (fod_image_in.dim(3)));
            Math::Vector<float> aPSF;
            Math::Matrix<float> aPSF_matrix (fod_image_in.dim(3), transformed_directions.columns());
            for (int i = 0; i < transformed_directions.columns(); ++i) {
              Point<float> dir (transformed_directions (0, i), transformed_directions (1, i), transformed_directions (2, i));
              aPSF_generator (aPSF, dir);
              for (int j = 0; j < fod_image_in.dim(3); ++j)
                aPSF_matrix (j, i) = aPSF[j];
            }
            Math::mult (reorient_transform, aPSF_matrix, fod_to_aPSF_weights_transform);
          }

          void operator() (const Image::Iterator& pos) {
            Image::voxel_assign (fod_voxel_in, pos, 0, 3);
            Image::voxel_assign (fod_voxel_out, pos, 0, 3);
            fod_voxel_in[3] = 0;
            if (fod_voxel_in.value() > 0) {
              for (fod_voxel_in[3] = 0; fod_voxel_in[3] < fod_in.size(); ++fod_voxel_in[3])
                fod_in[fod_voxel_in[3]] = fod_voxel_in.value();
              Math::mult (fod_out, reorient_transform, fod_in);
              for (fod_voxel_out[3] = 0; fod_voxel_out[3] < fod_out.size(); ++fod_voxel_out[3])
                 fod_voxel_out.value() = fod_out[fod_voxel_out[3]];
            }
          }

          void precompute_FOD_to_aPSF_weights_transform (const int num_SH,
                                                         const Math::Matrix<float>& directions,
                                                         Math::Matrix<float>& fod_to_aPSF_weights_transform) {
            Math::Matrix<float> aPSF_matrix (num_SH, directions.rows());
            Math::SH::aPSF<float> aPSF_generator (Math::SH::LforN (num_SH));
            Math::Vector<float> aPSF;
            for (size_t i = 0; i < directions.rows(); ++i) {
              Point<float> dir (directions (i, 0), directions (i, 1), directions (i, 2));
              aPSF_generator (aPSF, dir);
              aPSF_matrix.column(i) = aPSF;
            }
            Math::pinv (fod_to_aPSF_weights_transform, aPSF_matrix);
          }

        protected:
            FODVoxelType fod_voxel_in;
            FODVoxelType fod_voxel_out;
            Math::Matrix<float> reorient_transform;
            Math::Vector<float> fod_in;
            Math::Vector<float> fod_out;
        };



//        template <class FODVoxelType, class WarpVoxelType>
//        class WarpReorientKernel : public Base<FODVoxelType> {
//
//          public:
//            WarpReorientKernel (FODVoxelType& fod_voxel,
//                                const Math::Matrix<float>& directions,
//                                const WarpVoxelType& warp) :
//                                  warp_gradient (warp),
//                                  jacobian (3, 3) {}
//
//              void operator() (const Image::Iterator& pos) {
//                Image::voxel_assign (Base<FODVoxelType>::fod_voxel, pos, 0, 3);
//                Base<FODVoxelType>::fod_voxel[3] = 0;
//                if (Base<FODVoxelType>::fod_voxel.value() > 0) {
//                  jacobian.identity();
//                  Image::voxel_assign (warp_gradient, pos, 0, 3);
//                  for (size_t i = 0; i < 3; ++i) {
//                    warp_gradient.set_axis(i);
//                    for (size_t j = 0; j < 3; ++j) {
//                      warp_gradient[3] = j;
//                      jacobian(i, j) += warp_gradient.value();
//                    } // TODO check the rows/cols
//                  }
//                  // adjust for scanner coord TODO
//                  Math::LU::inv(Base<FODVoxelType>::transform, jacobian);
//                  Base<FODVoxelType>::reorient_FOD ();
//                }
//              }
//
//          private:
//            Adapter::Gradient1D<WarpVoxelType> warp_gradient;
//            Math::Matrix<float> jacobian;
//        };



        template <class FODVoxelType>
        void reorient (FODVoxelType& fod_vox_in,
                       FODVoxelType& fod_vox_out,
                       const Math::Matrix<float>& transform,
                       const Math::Matrix<float>& directions)
        {
          LinearReorientKernel<FODVoxelType> kernel (fod_vox_in, fod_vox_out, directions, transform);
          Image::ThreadedLoop loop (fod_vox_in, 1, 0, 3);
          loop.run (kernel);
        }



        template <class FODVoxelType>
        void reorient (std::string& progress_message,
                       FODVoxelType& fod_vox_in,
                       FODVoxelType& fod_vox_out,
                       const Math::Matrix<float>& transform,
                       const Math::Matrix<float>& directions)
        {
          LinearReorientKernel<FODVoxelType> kernel (fod_vox_in, fod_vox_out, directions, transform);
          Image::ThreadedLoop loop (progress_message, fod_vox_in, 1, 0, 3);
          loop.run (kernel);
        }


//
//        template <class FODVoxelType, class WarpVoxelType>
//        void reorient (FODVoxelType& fod_vox,
//                       const WarpVoxelType& warp,
//                       const Math::Matrix<float>& directions)
//        {
//          WarpReorientKernel<FODVoxelType, WarpVoxelType> kernel (fod_vox, directions, warp);
//          Image::ThreadedLoop loop (fod_vox, 1, 0, 3);
//          loop.run (kernel);
//        }
//
//
//
//        template <class FODVoxelType, class WarpVoxelType>
//        void reorient (std::string& progress_message,
//                       FODVoxelType& fod_vox,
//                       const WarpVoxelType& warp,
//                       const Math::Matrix<float>& directions)
//        {
//          WarpReorientKernel<FODVoxelType, WarpVoxelType> kernel (fod_vox, directions, warp);
//          Image::ThreadedLoop loop (progress_message, fod_vox, 1, 0, 3);
//          loop.run (kernel);
//        }


      }
    }
  }
}

#endif
