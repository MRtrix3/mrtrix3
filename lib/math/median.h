/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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

    namespace {
      template <typename X>
        inline bool not_a_number (X x) { 
          return false;
        }

      template <> inline bool not_a_number (float x) { return std::isnan (x); }
      template <> inline bool not_a_number (double x) { return std::isnan (x); }
    }



    template <class Container> 
    inline typename Container::value_type median (Container& list)
    {
        size_t num = list.size();
        // remove NaNs:
        for (size_t n = 0; n < num; ++n) {
          while (not_a_number (list[n]) && n < num) {
            --num;
            //std::swap (list[n], list[num]);
            // Commented std::swap to provide bool compatibility
            typename Container::value_type temp = list[num]; list[num] = list[n]; list[n] = temp;
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

