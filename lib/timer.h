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

#include <sys/time.h>
#include <time.h>

namespace MR
{

  class Timer
  {
    public:
      Timer () {
        start();
      }

      void start () {
        from = current_time();
      }
      double elapsed () {
        return (current_time()-from);
      }

      static double current_time () {
        struct timeval tv;
        gettimeofday (&tv, NULL);
        return (tv.tv_sec + 1.0e-6*float (tv.tv_usec));
      }

    protected:
      double from;
  };


  // a class to help perform operations at given time intervals
  class IntervalTimer : protected Timer
  {
    public:
      //! by default, fire at ~30 Hz - most monitors are 60Hz
      IntervalTimer (double interval = 0.0333333) :
        interval (interval),
        next_time (from + interval) { }

      //! return true if ready, false otherwise
      /*! Note that the timer immediately resets; next invocation will return
       * false until another interval has elapsed. */
      operator bool() {
        double now = current_time();
        if (now < next_time) 
          return false;
        from = now;
        next_time += interval;
        return true;
      }

    protected:
      const double interval;
      double next_time;
  };

}

#endif

