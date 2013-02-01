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

namespace MR
{
  namespace Math
  {

    template <typename ValueType> 
      class Median {
        public:
          typedef ValueType value_type;

          Median () {
            reset (0);
          }

          Median (size_t max_number) {
            reset (max_number);
          }

          void reset (size_t number_of_elements) {
            result = -std::numeric_limits<value_type>::infinity();
            count = number_of_elements;
            current_count = 0;
            median_index = number_of_elements/2 + 1;
            values.assign (median_index, value_type (0));
          }

          void operator+= (value_type val) {
            if (current_count < median_index) {
              values[current_count] = val;
              if (values[current_count] > result) 
                result = val;
              ++current_count;
            }
            else if (val < result) {
              size_t i;
              for (i = 0; values[i] != result; ++i);
              values[i] = val;
              result = -std::numeric_limits<value_type>::infinity();
              for (i = 0; i < median_index; i++)
                if (values[i] > result) result = values[i];
            }
          }

          value_type value () {

            if ((count+1) & 1) {
              value_type t = result = -std::numeric_limits<value_type>::infinity();
              for (size_t i = 0; i < median_index; ++i) {
                if (values[i] > result) {
                  t = result;
                  result = values[i];
                }
                else if (values[i] > t) t = values[i];
              }
              result = (result+t)/2.0;
            }

            return result;
          }

        protected:
          std::vector<value_type> values;
          value_type result;
          size_t count, current_count, median_index;
      };

  }
}


#endif

