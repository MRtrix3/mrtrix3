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
            throw Exception ("Error initialising histogram: number of buckets must be greater than 10");

          info ("Initialising histogram with " + str(num_buckets) + " buckets...");
          list.resize (num_buckets);

          value_type min, max;
          min_max (D, min, max);

          value_type step = (max-min) / value_type(num_buckets);

          for (size_t n = 0; n < list.size(); n++)
            list[n].value = min + step * (n + 0.5);

          Loop loop ("building histogram of \"" + D.name() + "\"...");
          for (loop.start (D); loop.ok(); loop.next (D)) {
            value_type val = D.value();
            if (finite (val) && val != 0.0) { 
              size_t pos = size_t ((val-min)/step);
              if (pos >= list.size()) pos = list.size()-1;
              list[pos].frequency++;
            }
          }
        }


        size_t      frequency (size_t index) const    { return (list[index].frequency); }
        value_type  value (size_t index) const        { return (list[index].value); }
        size_t      num () const                      { return (list.size()); }
        value_type  first_min () const {
          size_t p1 = 0;
          while (list[p1].frequency <= list[p1+1].frequency && p1+2 < list.size()) ++p1;
          for (size_t p = p1; p < list.size(); ++p) {
            if (2*list[p].frequency < list[p1].frequency) break;
            if (list[p].frequency >= list[p1].frequency) p1 = p;
          }

          size_t m1 (p1+1);
          while (list[m1].frequency >= list[m1+1].frequency && m1+2 < list.size()) ++m1;
          for (size_t m = m1; m < list.size(); ++m) {
            if (list[m].frequency > 2*list[m1].frequency) break;
            if (list[m].frequency <= list[m1].frequency) m1 = m;
          }

          return (list[m1].value);
        }

      protected:
        class Entry {
          public:
            Entry () : frequency (0), value (0.0) { }
            size_t  frequency;
            value_type  value;
        };

        std::vector<Entry> list;
        friend class Kernel;
    };


  }
}

#endif
