/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 17/02/12.

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

#ifndef __image_filter_gradient3D_h__
#define __image_filter_gradient3D_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/adapter/gradient1D.h"
#include "image/buffer_scratch.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {
      /** \addtogroup Filters
      @{ */

      /*! Compute the image gradients of a 3D image.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::Gradient3D gradient_filter (src);
       *
       * Image::Header header (src_data);
       * header.info() = gradient_filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * smooth_filter (src, dest);
       *
       * \endcode
       */
      class Gradient3D : public ConstInfo
      {

      public:
          template <class InputVoxelType>
            Gradient3D (const InputVoxelType& in) :
              ConstInfo (in) {
              axes_.resize(4);
              axes_[3].dim = 3;
              axes_[0].stride = 2;
              axes_[1].stride = 3;
              axes_[2].stride = 4;
              axes_[3].stride = 1;
              datatype_ = DataType::Float32;
          }

          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& in, OutputVoxelType& out) {
              Adapter::Gradient1D<InputVoxelType> gradient1D (in);
              out[3] = 0;
              threaded_copy_with_progress_message ("Computing x-axis gradient...", gradient1D, out, 2, 0, 3);
              out[3] = 1;
              gradient1D.set_axis(1);
              threaded_copy_with_progress_message ("Computing y-axis gradient...", gradient1D, out, 2, 0, 3);
              out[3] = 2;
              gradient1D.set_axis(2);
              threaded_copy_with_progress_message ("Computing z-axis gradient...", gradient1D, out, 2, 0, 3);
            }
      };
      //! @}
    }
  }
}


#endif
