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

namespace MR {
  namespace DataSet {

    namespace {
      template <class Set, typename T> class Kernel {
        public:
          Kernel (T& min_value, T& max_value) : min (min_value), max (max_value) { }
          void operator() (Set& D) { 
            T val = D.value();
            if (finite (val)) {
              if (min > val) min = val;
              if (max < val) max = val;
            }
          }
        private:
          T& min;
          T& max;
      };
    }

    template <class Set, typename T> 
      void min_max (Set& D, T& min, T& max, size_t from_axis = 0, size_t to_axis = SIZE_MAX)
      {
        min = T(INFINITY);
        max = T(-INFINITY);
        Kernel<Set,T> kernel (min, max);
        loop1 ("finding min/max of \"" + D.name() + "\"...", kernel, D, from_axis, to_axis);
      }

  }
}

#endif
