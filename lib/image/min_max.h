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

#ifndef __image_min_max_h__
#define __image_min_max_h__

#include "image/threaded_loop.h"

namespace MR
{
  namespace Image
  {

    //! \cond skip
    namespace {
      template <class VoxelType> 
      class __MinMax {
        public:
          typedef typename VoxelType::value_type value_type;

          __MinMax (value_type& overal_min, value_type& overal_max) :
            overal_min (overal_min), overal_max (overal_max),
            min (std::numeric_limits<value_type>::infinity()), 
            max (-std::numeric_limits<value_type>::infinity()) { 
              overal_min = min;
              overal_max = max;
            }
          ~__MinMax () {
            overal_min = std::min (overal_min, min);
            overal_max = std::max (overal_max, max);
          }

          void operator() (VoxelType& vox) {
            value_type val = vox.value();
            if (std::isfinite (val)) {
              if (val < min) min = val;
              if (val > max) max = val;
            }
          }

          value_type& overal_min;
          value_type& overal_max;
          value_type min, max;
      };
    }
    //! \endcond

    template <class InputVoxelType>
      inline void min_max (
          InputVoxelType& in, 
          typename InputVoxelType::value_type& min, 
          typename InputVoxelType::value_type& max, 
          size_t from_axis = 0, 
          size_t to_axis = std::numeric_limits<size_t>::max())
    {
      Image::ThreadedLoop ("finding min/max of \"" + shorten (in.name()) + "\"...", in)
        .run (__MinMax<InputVoxelType> (min, max), in);
    }

  }
}

#endif
