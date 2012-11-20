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
#include "image/transform.h"
#include "image/interp/cubic.h"
#include "image/iterator.h"
#include "point.h"


namespace MR
{
  namespace Image
  {
    namespace Registration
    {

      /** \addtogroup Registration
      @{ */

      /*! a thread kernel to compose two displacement fields
       *
       * Typical usage:
       * \code
       * Image::Buffer<float> composed_warp (warp1);
       * Image::Buffer<float>::voxel_type composed_warp_vox (composed_warp);
       *
       * WarpComposer <Image::Buffer<float>::voxel_type > compose_kernel (warp1, warp2, composed_warp);
       * ThreadedLoop (warp1, 1, 0, 3).run (compose_kernel);
       * \endcode
       */
      template <class FirstWarpVoxelType, class SecondWarpVoxelType, class OutputWarpVoxelType>
      class WarpComposer
      {

        public:

          WarpComposer (FirstWarpVoxelType& first_warp,
                        SecondWarpVoxelType& second_warp,
                        OutputWarpVoxelType& output_warp) :
            first_warp (first_warp),
            output_warp (output_warp),
            output_transform (output_warp),
            second_warp (second_warp) { }


          void operator () (const Iterator& pos) {

            voxel_assign (output_warp, pos, 0, 3);
            voxel_assign (first_warp, pos, 0, 3);

            Point<float> point = output_transform.voxel2scanner (pos);
            Point<float> displacement;
            for (size_t dim = 0; dim < 3; ++dim) {
              first_warp[3] = dim;
              displacement[dim] = first_warp.value();
            }
            point += displacement;
            second_warp.scanner (point);
            for (size_t dim = 0; dim < 3; ++dim) {
              second_warp[3] = dim;
              output_warp[3] = dim;
              output_warp.value() = displacement[dim] + second_warp.value();
            }
          }


        protected:
          FirstWarpVoxelType first_warp;
          OutputWarpVoxelType output_warp;
          Transform output_transform;
          Interp::Cubic<SecondWarpVoxelType> second_warp;

      };
      //! @}
    }
  }
}


#endif
