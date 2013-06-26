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

        template <class FODVoxelType>
        class Base {

          public:
            Base (FODVoxelType& fod_voxel,
                  const Math::Matrix<float>& directions,
                  const Math::Matrix<float>& fod_to_aPSF_weights_transform) :
                  fod_voxel (fod_voxel),
                  directions (directions),
                  fod_to_aPSF_weights_transform (fod_to_aPSF_weights_transform),
                  aPSF_generator (Math::SH::LforN(fod_voxel.dim(3))){ }

          protected:
            inline void reorient_FOD () {
              // Compute the aPSF weights
              Math::Vector<float> fod (fod_voxel.address(), fod_voxel.dim(3));
              Math::Vector<float> weights;
              Math::mult (weights, fod_to_aPSF_weights_transform, fod);

              Math::Matrix<float> transformed_directions;
              Math::mult (transformed_directions, transform, directions);
              fod.zero();
              // for each direction, compute an apodised PSF, add it to the FOD with the weight
              for (int i = 0; i < directions.rows(); ++i) {
                Math::Vector<float> aPSF;
                Point<float> dir (directions (i, 0), directions (i, 1), directions (i, 2));
                aPSF_generator (aPSF, dir);
                aPSF *= weights[i];
                fod += aPSF;
              }
            }

            FODVoxelType fod_voxel;
            Math::Matrix<float> directions;
            Math::Matrix<float> fod_to_aPSF_weights_transform;
            Math::Matrix<float> transform;
            Math::SH::aPSF<float> aPSF_generator;
        };




        template <class FODVoxelType>
        class LinearReorientKernel : public Base<FODVoxelType> {

        public:
          LinearReorientKernel (FODVoxelType& fod_voxel,
                                const Math::Matrix<float>& directions,
                                const Math::Matrix<float>& fod_to_aPSF_weights_transform,
                                const Math::Matrix<float>& transform) :
                                  Base<FODVoxelType> (fod_voxel, directions, fod_to_aPSF_weights_transform) {}

            void operator() (const Image::Iterator& pos) {
              Image::voxel_assign (Base<FODVoxelType>::fod_voxel, pos, 0, 3);
              Base<FODVoxelType>::fod_voxel[3] = 0;
              if (Base<FODVoxelType>::fod_voxel.value() > 0)
                Base<FODVoxelType>::reorient_FOD ();
            }
        };



        template <class FODVoxelType, class WarpVoxelType>
        class WarpReorientKernel : public Base<FODVoxelType> {

          public:
            WarpReorientKernel (FODVoxelType& fod_voxel,
                                const Math::Matrix<float>& directions,
                                const Math::Matrix<float>& fod_to_aPSF_weights_transform,
                                const WarpVoxelType& warp) :
                                  Base<FODVoxelType> (fod_voxel, directions, fod_to_aPSF_weights_transform),
                                  warp_gradient (warp) {}

              void operator() (const Image::Iterator& pos) {
                Image::voxel_assign (Base<FODVoxelType>::fod_voxel, pos, 0, 3);
                Base<FODVoxelType>::fod_voxel[3] = 0;
                if (Base<FODVoxelType>::fod_voxel.value() > 0) {
                  Base<FODVoxelType>::transform.identity();
                  Image::voxel_assign (warp_gradient, pos, 0, 3);
                  for (size_t i = 0; i < 3; ++i) {
                    warp_gradient.set_axis(i);
                    for (size_t j = 0; j < 3; ++j) {
                      warp_gradient[3] = j;
                      Base<FODVoxelType>::transform(i, j) += warp_gradient.value();
                    } // TODO check the rows/cols
                  }
                  // adjust for scanner coord TODO
                  Base<FODVoxelType>::reorient_FOD ();
                }
              }

          private:
            Adapter::Gradient1D<WarpVoxelType> warp_gradient;
        };

        void precompute_FOD_to_aPSF_weights_transform (const int num_SH,
                                                       const Math::Matrix<float>& directions,
                                                       Math::Matrix<float>& fod_to_aPSF_weights_transform) {
          Math::Matrix<float> aPSF_matrix (num_SH, directions.rows());
          Math::SH::aPSF<float> aPSF_generator (Math::SH::LforN (num_SH));
          for (size_t i = 0; i < directions.rows(); ++i) {
            Point<float> dir (directions (i, 0), directions (i, 1), directions (i, 2));
            Math::Vector<float> aPSF;
            aPSF_generator (aPSF, dir);
            fod_to_aPSF_weights_transform.column(i) = aPSF;
          }
          Math::LU::inv (fod_to_aPSF_weights_transform, aPSF_matrix);
        }


        template <class FODVoxelType>
        void reorient (FODVoxelType& fod_vox,
                       const Math::Matrix<float>& transform,
                       const Math::Matrix<float>& directions)
        {
          Math::Matrix<float> fod_to_aPSF_weights_transform;
          precompute_FOD_to_aPSF_weights_transform (fod_vox.dim(3), directions, fod_to_aPSF_weights_transform);
          LinearReorientKernel<FODVoxelType> kernel (fod_vox,
                                                     transform,
                                                     directions,
                                                     fod_to_aPSF_weights_transform);
          Image::ThreadedLoop loop (fod_vox, 1, 0, 3);
          loop.run (kernel);
        }

        template <class FODVoxelType>
        void reorient (std::string& progress_message,
                       FODVoxelType& fod_vox,
                       const Math::Matrix<float>& transform,
                       const Math::Matrix<float>& directions)
        {
          Math::Matrix<float> fod_to_aPSF_weights_transform;
          precompute_FOD_to_aPSF_weights_transform (fod_vox.dim(3), directions, fod_to_aPSF_weights_transform);
          LinearReorientKernel<FODVoxelType> kernel (fod_vox,
                                                     transform,
                                                     directions,
                                                     fod_to_aPSF_weights_transform);
          Image::ThreadedLoop loop (progress_message, fod_vox, 1, 0, 3);
          loop.run (kernel);
        }

        template <class FODVoxelType, class WarpVoxelType>
        void reorient (FODVoxelType& fod_vox,
                       const WarpVoxelType& warp,
                       const Math::Matrix<float>& directions)
        {
          Math::Matrix<float> fod_to_aPSF_weights_transform;
          precompute_FOD_to_aPSF_weights_transform (fod_vox.dim(3), directions, fod_to_aPSF_weights_transform);
          WarpReorientKernel<FODVoxelType, WarpVoxelType> kernel (fod_vox,
                                                                  warp,
                                                                  directions,
                                                                  fod_to_aPSF_weights_transform);
          Image::ThreadedLoop loop (fod_vox, 1, 0, 3);
          loop.run (kernel);
        }

        template <class FODVoxelType, class WarpVoxelType>
        void reorient (std::string& progress_message,
                       FODVoxelType& fod_vox,
                       const WarpVoxelType& warp,
                       const Math::Matrix<float>& directions)
        {
          Math::Matrix<float> fod_to_aPSF_weights_transform;
          precompute_FOD_to_aPSF_weights_transform (fod_vox.dim(3), directions, fod_to_aPSF_weights_transform);
          WarpReorientKernel<FODVoxelType, WarpVoxelType> kernel (fod_vox,
                                                                  warp,
                                                                  directions,
                                                                  fod_to_aPSF_weights_transform);
          Image::ThreadedLoop loop (progress_message, fod_vox, 1, 0, 3);
          loop.run (kernel);
        }


      }
    }
  }
}

#endif
