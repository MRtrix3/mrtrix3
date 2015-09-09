/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __image_filter_median3D_h__
#define __image_filter_median3D_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/adapter/median3D.h"
#include "image/filter/base.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {
      /** \addtogroup Filters
      @{ */

      /*! Smooth images using median filtering.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * auto src = src_data.voxel();
       * Image::Filter::Median median_filter (src);
       *
       * Image::Header header (src_data);
       * header.info() = median_filter.info();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * auto dest = dest_data.voxel();
       *
       * median_filter (src, dest);
       *
       * \endcode
       */
      class Median : public Base
      {

        public:
          template <class InfoType>
          Median (const InfoType& in) :
              Base (in),
              extent_ (1,3) { }

          template <class InfoType>
          Median (const InfoType& in, const std::string& message) :
              Base (in, message),
              extent_ (1,3) { }

          template <class InfoType>
          Median (const InfoType& in, const std::vector<int>& extent) :
              Base (in),
              extent_ (extent) { }

          template <class InfoType>
          Median (const InfoType& in, const std::string& message, const std::vector<int>& extent) :
              Base (in, message),
              extent_ (extent) { }

          //! Set the extent of median filtering neighbourhood in voxels.
          //! This must be set as a single value for all three dimensions
          //! or three values, one for each dimension. Default 3x3x3.
          void set_extent (const std::vector<int>& extent) {
            for (size_t i = 0; i < extent.size(); ++i) {
              if (!(extent[i] & int (1)))
                throw Exception ("expected odd number for extent");
              if (extent[i] < 0)
                throw Exception ("the kernel extent must be positive");
            }
            extent_ = extent;
          }

          template <class InputVoxelType, class OutputVoxelType>
          void operator() (InputVoxelType& in, OutputVoxelType& out) {
              Adapter::Median3D<InputVoxelType> median (in, extent_);
              if (message.size())
                threaded_copy_with_progress_message (message, median, out);
              else
                threaded_copy (median, out);
          }

      protected:
          std::vector<int> extent_;
      };
      //! @}
    }
  }
}


#endif
