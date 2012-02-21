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

#ifndef __image_filter_gaussian3D_h__
#define __image_filter_gaussian3D_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/adapter/gaussian1D.h"
#include "image/buffer_scratch.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {
      /** \addtogroup Filters
      @{ */

      /*! Smooth images using a Gaussian kernel.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::Gaussian3D smooth_filter (src);
       *
       * std::vector<float> stdev (1);
       * stdev[0] = 2;
       * smooth_filter.set_stdev (stdev);
       *
       * Image::Header header (src_data);
       * header.info() = smooth_filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * smooth_filter (src, dest);
       *
       * \endcode
       */
      class Gaussian3D : public ConstInfo
      {

      public:
          template <class InputVoxelType>
            Gaussian3D (const InputVoxelType& in) :
              ConstInfo (in),
              extent_ (3,0),
              stdev_ (3,1) { }

          template <class InputVoxelType>
            Gaussian3D (const InputVoxelType& in,
                        const std::vector<int>& extent,
                        const std::vector<float>& stdev) :
              ConstInfo (in),
              extent_(3),
              stdev_(3) {
              set_extent (extent);
              set_stdev (stdev);
          }

          //! Set the extent of smoothing kernel in voxels.
          //! This must be set as a single value for all three dimensions
          //! or three values, one for each dimension. (Default: 4 standard deviations)
          void set_extent (const std::vector<int>& extent) {
            if (extent.size() == 1) {
              extent_[0] = extent[0];
              extent_[1] = extent[0];
              extent_[2] = extent[0];
            } else {
              extent_ = extent;
            }
          }

          //! Set the standard deviation of the Gaussian defined in mm.
          //! This must be set as a single value for all three dimensions
          //! or three values, one for each dimension. (Default: 1x1x1)
          void set_stdev (const std::vector<float>& stdev) {
            if (stdev.size() == 1) {
              stdev_[0] = stdev[0];
              stdev_[1] = stdev[0];
              stdev_[2] = stdev[0];
            } else {
              stdev_ = stdev;
            }
          }

          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& in, OutputVoxelType& out) {

              typedef typename OutputVoxelType::value_type value_type;

              Image::BufferScratch<value_type> x_scratch (out);
              typename Image::BufferScratch<value_type>::voxel_type x_voxel (x_scratch);
              Adapter::Gaussian1D<InputVoxelType> x_gaussian (in, stdev_[0], 0, extent_[0]);
              threaded_copy_with_progress_message ("Filtering x-axis...", x_gaussian, x_voxel);

              typedef typename Image::BufferScratch<value_type>::voxel_type buffer_voxel_type;
              Adapter::Gaussian1D<buffer_voxel_type> y_gaussian (x_voxel, stdev_[1], 1, extent_[1]);
              Image::BufferScratch<value_type> y_scratch (out);
              typename Image::BufferScratch<value_type>::voxel_type y_voxel (y_scratch);
              threaded_copy_with_progress_message ("Filtering y-axis...", y_gaussian, y_voxel);

              Adapter::Gaussian1D<buffer_voxel_type> z_gaussian (y_voxel, stdev_[2], 2, extent_[2]);
              threaded_copy_with_progress_message ("Filtering z-axis...", z_gaussian, out);
            }

      protected:
          std::vector<int> extent_;
          std::vector<float> stdev_;
      };
      //! @}
    }
  }
}


#endif
