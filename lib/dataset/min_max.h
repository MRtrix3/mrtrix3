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

#ifndef __dataset_min_max_h__
#define __dataset_min_max_h__

#include "dataset/loop.h"

namespace MR {
  namespace DataSet {

    template <class Set, typename T> 
      void min_max (Set& D, T& min, T& max, size_t from_axis = 0, size_t to_axis = SIZE_MAX)
      {
        min = T(INFINITY);
        max = T(-INFINITY);
        LoopInOrder loop (D, "finding min/max of \"" + shorten (D.name()) + "\"...");
        for (loop.start (D); loop.ok(); loop.next (D)) {
          T val = D.value();
          if (finite (val)) {
            if (min > val) min = val;
            if (max < val) max = val;
          }
        }
      }

  }
}

#endif
