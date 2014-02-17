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

