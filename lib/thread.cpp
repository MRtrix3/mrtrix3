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

#include "thread.h"
#include "file/config.h"

namespace MR {
  namespace Thread {

    const pthread_attr_t* default_attributes () {
      static pthread_attr_t* attr = NULL;
      if (!attr) {
        attr = new pthread_attr_t;
        pthread_attr_init (attr);
        pthread_attr_setdetachstate (attr, PTHREAD_CREATE_JOINABLE);
      }
      return (attr);
    }

    size_t num_cores () {
      static size_t N = File::Config::get_int ("NumberOfThreads", 1);
      return (N);
    }

  }
}

