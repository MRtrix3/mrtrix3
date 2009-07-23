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

namespace MR {

  class Timer {
    public:
      Timer () { start(); }

      void start () { from = current_time(); }
      double elapsed () { return (current_time()-from); }

    protected:
      double from;

      double current_time () { 
        struct timeval tv;
        gettimeofday (&tv, NULL);
        return (tv.tv_sec + 1.0e-6*float(tv.tv_usec)); 
      }
  };

}

#endif

