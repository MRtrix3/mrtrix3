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

