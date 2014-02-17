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

