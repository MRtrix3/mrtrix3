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

#ifndef __math_median_h__
#define __math_median_h__

#include <vector>
#include <limits>


#include <algorithm>

#include "types.h"


namespace MR
{
  namespace Math
  {



    template <class Container> 
      inline typename Container::value_type median (Container& list) 
      {
        size_t num = list.size();
        // remove NaNs:
        for (size_t n = 0; n < num; ++n) {
          while (isnan (list[n]) && n < num) {
            --num;
            std::swap (list[n], list[num]);
          }
        }
        if (!num)
          return std::numeric_limits<typename Container::value_type>::quiet_NaN();

        size_t middle = num/2;
        std::nth_element (list.begin(), list.begin()+middle, list.begin()+num);
        typename Container::value_type med_val = list[middle];
        if (!(num&1U)) {
          --middle;
          std::nth_element (list.begin(), list.begin()+middle, list.begin()+middle+1);
          med_val = (med_val + list[middle])/2.0;
        }
        return med_val;
      }


  }
}


#endif

