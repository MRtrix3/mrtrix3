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

#include "app.h"
#include "thread.h"
#include "file/config.h"

namespace MR {
  namespace Thread {

    pthread_attr_t default_attr;
    size_t number_of_threads = 0;

    bool initialised () { return (number_of_threads); }


    namespace {
      Mutex print_mutex;

      void cmdline_thread_print (const std::string& msg) { Mutex::Lock lock (print_mutex); std::cout << msg; }

      void cmdline_thread_error (const std::string& msg) 
      {
        Mutex::Lock lock (print_mutex);
        if (App::log_level) std::cerr << App::name() << ": " << msg << "\n"; 
      }

      void cmdline_thread_info  (const std::string& msg) 
      { 
        Mutex::Lock lock (print_mutex);
        if (App::log_level > 1) std::cerr << App::name() << " [INFO]: " <<  msg << "\n"; 
      }

      void cmdline_thread_debug (const std::string& msg)
      { 
        Mutex::Lock lock (print_mutex);
        if (App::log_level > 2) std::cerr << App::name() << " [DEBUG]: " <<  msg << "\n"; 
      }
    }


    void init () {
      assert (!initialised());
      pthread_attr_init (&default_attr);
      pthread_attr_setdetachstate (&default_attr, PTHREAD_CREATE_JOINABLE);
      number_of_threads = File::Config::get_int ("NumberOfProcessors", 1);
      print = cmdline_thread_print;
      error = cmdline_thread_error;
      info = cmdline_thread_info;
      debug = cmdline_thread_debug;
      debug ("multi-threading initialised, assuming " + str(number_of_threads) + " processor" + (number_of_threads > 1 ? "s" : ""));
    }

    size_t num_cores () { assert (initialised()); return (number_of_threads); }
    const pthread_attr_t* default_attributes() { assert (initialised()); return (&default_attr); }

  }
}

