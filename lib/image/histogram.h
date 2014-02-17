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


#ifndef __image_histogram_h__
#define __image_histogram_h__

#include "image/min_max.h"

namespace MR
{
  namespace Image
  {

    template <class Set> class Histogram
    {
      public:
        typedef typename Set::value_type value_type;

        Histogram (Set& D, size_t num_buckets=100) {
          if (num_buckets < 10)
            throw Exception ("Error initialising histogram: number of buckets must be greater than 10");

          INFO ("Initialising histogram with " + str (num_buckets) + " buckets...");
          list.resize (num_buckets);

          value_type min, max;
          min_max (D, min, max);

          value_type step = (max-min) / value_type (num_buckets);

          for (size_t n = 0; n < list.size(); n++)
            list[n].value = min + step * (n + 0.5);

          MR::Image::LoopInOrder loop (D, "building histogram of \"" + shorten (D.name()) + "\"...");
          for (loop.start (D); loop.ok(); loop.next (D)) {
            value_type val = D.value();
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
            if(probability > 0.99 / totalFrequency)
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
}

#endif
