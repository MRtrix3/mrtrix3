/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 19/11/12

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

#ifndef __image_filter_warp_composer_h__
#define __image_filter_warp_composer_h__

#include "image/info.h"


namespace MR
{
  namespace Image
  {
    namespace Registration
    {

      /** \addtogroup Registration
      @{ */

      /*! a thread kernel to compose multiple displacement fields
       *
       * Typical usage:
       * \code
       * Image::Buffer<float> composed_warp (warp1);
       * Image::Buffer<float>::voxel_type composed_warp_vox (composed_warp);
       *
       * WarpComposerThreadKernel <Image::Buffer<float>::value_type > compose_kernel (warp1, composed_warp);
       * compose_kernel.add_warp (warp2);
       * compose_kernel.add_warp (warp3);
       * ThreadedLoop (warp1, 1, 0, 3).run (compose_kernel);
       * \endcode
       */
      template <class WarpVoxelType>
      class WarpComposerThreadKernel
      {

        public:

          WarpComposerThreadKernel (WarpVoxelType& first_warp, WarpVoxelType& output_warp) :
            first_warp (first_warp),
            output_warp (output_warp),
            output_transform (output_warp) {}

          void add_warp (WarpVoxelType& warp) {
            warps.push_back (new Image::Interp::Linear<WarpVoxelType> (warp, 0.0));
          }

          void operator () (const Iterator& pos) {
            voxel_assign (output_warp, pos, 0, 3);
            voxel_assign (first_warp, pos, 0, 3);
            Point<float> point = output_transform.voxel2scanner (pos);
            Point<float> displacement;
            for (size_t dim = 0; dim < 3; ++dim) {
              first[3] = dim;
              displacement[dim] = first_warp.value();
            }
            point += displacement;
            for (size_t w = 0; w < warps.size(); ++w) {
              warps[w].scanner (point);
              for (size_t dim = 0; dim < 3; ++dim) {
                interp[3] = dim;
                displacement[dim] = warps[w].value();
              }
              point += displacement;
            }

            for (size_t dim = 0; dim < 3; ++dim) {
              output_warp[3] = dim;
              output_warp.value() = point[dim];
            }
          }

        protected:
          WarpVoxelType first_warp;
          WarpVoxelType output_warp;
          Image::Transform output_transform;
          std::vector<Ptr<Image::Interp::Linear<WarpVoxelType> > > warps;

      };
      //! @}
    }
  }
}


#endif
