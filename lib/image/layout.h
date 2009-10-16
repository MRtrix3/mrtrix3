/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 16/10/09.

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

#ifndef __image_layout_h__
#define __image_layout_h__

#include "types.h"
#include "mrtrix.h"

namespace MR {
  namespace Image {

    //! \addtogroup Image 
    // @{
    
    //! \todo document layout class
    class Layout {
      public:
        Layout () : axis (0), dir (1) { }
        Layout (size_t axis_index, ssize_t direction) : axis (axis_index), dir (direction) { }
        size_t axis;
        ssize_t dir;
    };

    inline void get_layout (Layout* layout, const Axes& axes) {
      for (size_t i = 0; i < axes.ndim(); ++i) {
        layout[axes[i].order].axis = i;
        layout[axes[i].order].dir = axes[i].forward ? 1 : -1;
      }
    }

    //! @}
  }
}

#endif



