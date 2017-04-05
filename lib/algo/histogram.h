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

#ifndef __image_histogram_h__
#define __image_histogram_h__

#include <vector>

#include "algo/loop.h"
#include "algo/min_max.h"

namespace MR
{

    template <class Set> class Histogram
    {
      public:
        using value_type = typename Set::value_type;

        Histogram (Set& D, const size_t num_buckets=100) {
          if (num_buckets < 10)
            throw Exception ("Error initialising histogram: number of buckets must be greater than 10");

          INFO ("Initialising histogram with " + str (num_buckets) + " buckets");
          list.resize (num_buckets);

          value_type min, max;
          min_max (D, min, max);

          value_type step = (max-min) / value_type (num_buckets);

          for (size_t n = 0; n < list.size(); n++)
            list[n].value = min + step * (n + 0.5);

          for (auto l = Loop("building histogram of \"" + shorten (D.name()) + "\"", D) (D); l; ++l) {
            const value_type val = D.value();
            if (std::isfinite (val) && val != 0.0) {
              size_t pos = size_t ( (val-min) /step);
              if (pos >= list.size()) pos = list.size()-1;
              list[pos].frequency++;
            }
          }
        }


        size_t frequency (size_t index) const {
          return list[index].frequency;
        }
        value_type value (size_t index) const {
          return list[index].value;
        }
        size_t num () const {
          return list.size();
        }
        value_type first_min () const {
          size_t p1 = 0;
          while (list[p1].frequency <= list[p1+1].frequency && p1+2 < list.size())
            ++p1;
          for (size_t p = p1; p < list.size(); ++p) {
            if (2*list[p].frequency < list[p1].frequency)
              break;
            if (list[p].frequency >= list[p1].frequency)
              p1 = p;
          }

          size_t m1 (p1+1);
          while (list[m1].frequency >= list[m1+1].frequency && m1+2 < list.size())
            ++m1;
          for (size_t m = m1; m < list.size(); ++m) {
            if (list[m].frequency > 2*list[m1].frequency)
              break;
            if (list[m].frequency <= list[m1].frequency)
              m1 = m;
          }

          return list[m1].value;
        }

        float entropy () const {
          int totalFrequency = 0;
          for (size_t i = 0; i < list.size(); i++)
            totalFrequency += list[i].frequency;
          float imageEntropy = 0;

          for (size_t i = 0; i < list.size(); i++){
            double probability = static_cast<double>(list[i].frequency) / static_cast<double>(totalFrequency);
            if (probability > 0.99 / totalFrequency)
              imageEntropy += -probability * log(probability);
          }
          return imageEntropy;
        }

      protected:
        class Entry
        {
          public:
            Entry () : frequency (0), value (0.0) { }
            size_t  frequency;
            value_type  value;
        };

        std::vector<Entry> list;
        friend class Kernel;
    };


}

#endif
