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

#include "image.h"
#include "algo/threaded_copy.h"
#include "adapter/median3D.h"
#include "filter/base.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! Smooth images using median filtering.
     *
     * Typical usage:
     * \code
     * auto input = Image<float>::open (argument[0]);
     * Filter::Median median_filter (input);
     * auto output = Image<float>::create (argument[1], median_filter);
     * median_filter (input, output);
     *
     * \endcode
     */
    class Median : public Base
    {

      public:
        template <class HeaderType>
        Median (const HeaderType& in) :
            Base (in),
            extent (1,3) {
          datatype() = DataType::Float32;
        }

        template <class HeaderType>
        Median (const HeaderType& in, const std::vector<int>& extent) :
            Base (in),
            extent (extent) {
          datatype() = DataType::Float32;
        }

        //! Set the extent of median filtering neighbourhood in voxels.
        //! This must be set as a single value for all three dimensions
        //! or three values, one for each dimension. Default 3x3x3.
        void set_extent (const std::vector<int>& ext) {
          for (size_t i = 0; i < ext.size(); ++i) {
            if (!(ext[i] & int (1)))
              throw Exception ("expected odd number for extent");
            if (ext[i] < 0)
              throw Exception ("the kernel extent must be positive");
          }
          extent = ext;
        }

        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& in, OutputImageType& out) {
          Adapter::Median3D<InputImageType> median (in, extent);
          if (message.size())
            threaded_copy_with_progress_message (message, median, out);
          else
            threaded_copy (median, out);
        }

    protected:
        std::vector<int> extent;
    };
    //! @}
  }
}


#endif
