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

#include "progressbar.h"

namespace MR {
  namespace ProgressBar {

    std::string message; 
    size_t current_val = 0, percent = 0;
    float multiplier = 0.0;
    bool display = true;
    bool stop = false;
    Timer stop_watch;

    void (*init_func) () = NULL;
    void (*display_func) () = NULL;
    void (*done_func) () = NULL;


    void init (size_t target, const std::string& msg)
    {
      stop = false;
      message = msg;
      if (target) multiplier = 100.0/((float) target);
      else multiplier = NAN;
      current_val = percent = 0;
      if (isnan (multiplier)) stop_watch.start();
      init_func ();
      if (display) display_func ();
    }

  }
}

