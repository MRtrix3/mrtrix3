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

#ifndef __dataset_histogram_h__
#define __dataset_histogram_h__

#include "dataset/min_max.h"

namespace MR {
  namespace DataSet {

    template <class Set> class Histogram {
      public:
        typedef typename Set::value_type value_type;

        Histogram (Set& D, size_t num_buckets=100) {
          if (num_buckets < 10) 
            throw Exception ("Error initialising Histogram: number of buckets must be greater than 10");

          info ("Initialising histogram with " + str(num_buckets) + " buckets...");
          list.resize (num_buckets);

          value_type min, max;
          min_max (D, min, max);

          value_type step = (max-min) / value_type(num_buckets);

          for (size_t n = 0; n < list.size(); n++)
            list[n].value = min + step * (n + 0.5);

          Kernel kernel (list, min, step);
          loop1 ("building histogram of \"" + D.name() + "\"...", kernel, D);
        }


        size_t      frequency (size_t index) const    { return (list[index].frequency); }
        value_type  value (size_t index) const        { return (list[index].value); }
        size_t      num () const                      { return (list.size()); }
        value_type  first_min () const {
          size_t first_peak (0), first_peak_index (0), second_peak (0), second_peak_index (0),
                 first_minimum (0), first_min_index (0), range_step (list.size()/20), range (range_step), index;

          for (index = 0; index < range; index++) {
            if (list[index].frequency > first_peak) {
              first_peak = list[index].frequency;
              first_minimum = first_peak;
              first_min_index = first_peak_index = index;
            }
          }

          range = first_peak_index + range_step;

          for (index = first_peak_index; index < range; index++) {
            if (list[index].frequency < first_minimum) {
              first_minimum = list[index].frequency;
              first_min_index = second_peak_index = index;
            }
          }

          range = first_min_index + range_step;

          for (index = first_min_index; index < range; index++) {
            if (list[index].frequency > second_peak) {
              second_peak = list[index].frequency;
              second_peak_index = index;
            }
          }

          return (list[first_min_index].value);
        }

      protected:
        class Entry {
          public:
            Entry () : frequency (0), value (0.0) { }
            size_t  frequency;
            value_type  value;
        };

        class Kernel {
          public:
            Kernel (std::vector<Entry>& hist, value_type min, value_type step) : list (hist), M (min), S (step) { }
            void operator() (Set& D) { 
              value_type val = D.value();
              if (finite (val)) { 
                size_t pos = size_t ((val-M)/S);
                if (pos >= list.size()) pos = list.size()-1;
                list[pos].frequency++;
              }
            }
          private:
            std::vector<Entry>& list;
            value_type M, S;
        };

        std::vector<Entry> list;
        friend class Kernel;
    };


  }
}

#endif
