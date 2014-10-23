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

#ifndef __timer_h__
#define __timer_h__

#include <chrono>

namespace MR
{

  class Timer
  {
    public:
      Timer () {
        start();
      }

      void start () {
        from = std::chrono::high_resolution_clock::now();
      }
      double elapsed () {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - from).count() * 1.0e-9;
      }

      static double current_time () {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() * 1.0e-9;
      }

    protected:
      std::chrono::high_resolution_clock::time_point from;
  };




  // a class to help perform operations at given time intervals
  class IntervalTimer : protected Timer
  {
    public:
      //! by default, fire at ~30 Hz - most monitors are 60Hz
      IntervalTimer (double time_interval = 0.0333333) :
        interval (std::chrono::duration_cast<std::chrono::high_resolution_clock::duration> (std::chrono::nanoseconds (std::chrono::nanoseconds::rep (1.0e9*time_interval)))),
        next_time (from + interval) { }

      //! return true if ready, false otherwise
      /*! Note that the timer immediately resets; next invocation will return
       * false until another interval has elapsed. */
      operator bool() {
        auto now = std::chrono::high_resolution_clock::now();
        if (now < next_time) 
          return false;
        from = now;
        next_time += interval;
        return true;
      }

    protected:
      const std::chrono::high_resolution_clock::duration interval;
      std::chrono::high_resolution_clock::time_point next_time;
  };

}

#endif

