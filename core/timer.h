/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __timer_h__
#define __timer_h__

#include <chrono>

#define NOMEMALIGN

namespace MR
{

  class Timer { NOMEMALIGN
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
  class IntervalTimer : protected Timer { NOMEMALIGN
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

