/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

