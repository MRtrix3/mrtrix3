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

#ifndef __progressbar_h__
#define __progressbar_h__

#include <string>

#include "mrtrix.h"
#include "timer.h"
#include "types.h"
#include "math/math.h"

#define BUSY_INTERVAL 0.1

namespace MR {

  class ProgressInfo 
  {
    public:
      ProgressInfo () : as_percentage (false), value (0), data (NULL) { }
      ProgressInfo (const std::string& message, bool has_target) :
        as_percentage (has_target), value (0), text (message), data (NULL) { }

      const bool as_percentage;
      size_t value;
      const std::string text;
      void* data;
  };


  class ProgressBar : private ProgressInfo
  {
    public:

      ProgressBar () : show (0) { }

      ProgressBar (const std::string& text, size_t target = 0) : 
        ProgressInfo (text, target),
        show (display),
        current_val (0) {
          if (show) {
            if (as_percentage) 
              set_max (target);
            else 
              next_val.d = BUSY_INTERVAL;
          }
        }

      ~ProgressBar () 
      { 
        if (show)
          done_func (*this);
      }

      operator bool () const { return (show); }
      bool operator! () const { return (!show); }

      void set_max (size_t target) 
      {
        assert (target);
        assert (as_percentage);
        multiplier = 0.01 * target;
        next_val.i = multiplier;
        if (!next_val.i)
          next_val.i = 1;
      }

      void operator++ () 
      {
        if (show) {
          if (as_percentage) {
            ++current_val; 
            if (current_val >= next_val.i) {
              value = next_val.i / multiplier;
              next_val.i = (value+1) * multiplier;
              while (next_val.i <= current_val) 
                ++next_val.i;
              display_func (*this);
            }
          }
          else {
            double time = timer.elapsed();
            if (time >= next_val.d) {
              value = time / BUSY_INTERVAL;
              do {
                next_val.d += BUSY_INTERVAL;
              } while (next_val.d <= time);
              display_func (*this);
            }
          }
        }
      }

      void operator++ (int unused) { return ((*this)++); }

      static bool display;
      static void (*display_func) (ProgressInfo& p);
      static void (*done_func) (ProgressInfo& p);

    private:
      const bool show;
      size_t current_val;
      union { size_t i; double d; } next_val;
      float multiplier;
      Timer timer;
  };

}

#endif

