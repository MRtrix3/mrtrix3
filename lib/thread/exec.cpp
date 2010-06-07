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
#include "thread/exec.h"
#include "thread/mutex.h"
#include "file/config.h"

namespace MR {
  namespace Thread {

    pthread_attr_t* Exec::default_attributes = NULL;
    pthread_t Exec::main_thread;
    Mutex Exec::mutex;

    void (*Exec::previous_print) (const std::string& msg);
    void (*Exec::previous_error) (const std::string& msg);
    void (*Exec::previous_info)  (const std::string& msg);
    void (*Exec::previous_debug) (const std::string& msg);

    void Exec::thread_print (const std::string& msg) { Mutex::Lock lock (mutex); std::cout << msg; }

    void Exec::thread_error (const std::string& msg) {
      if (App::log_level) {
        Mutex::Lock lock (mutex);
        std::cerr << App::name() << ": " << msg << "\n"; 
      }
    }

    void Exec::thread_info  (const std::string& msg) { 
      if (App::log_level > 1) {
        Mutex::Lock lock (mutex);
        std::cerr << App::name() << " [INFO]: " <<  msg << "\n"; 
      }
    }

    void Exec::thread_debug (const std::string& msg) { 
      if (App::log_level > 2) {
        Mutex::Lock lock (mutex);
        std::cerr << App::name() << " [DEBUG]: " <<  msg << "\n"; 
      }
    }


    void Exec::init () {
      assert (!default_attributes);
      responsible = true;
      main_thread = pthread_self();

      default_attributes = new pthread_attr_t;
      pthread_attr_init (default_attributes);
      pthread_attr_setdetachstate (default_attributes, PTHREAD_CREATE_JOINABLE);

      previous_print = print;
      previous_error = error;
      previous_info = info;
      previous_debug = debug;

      print = thread_print;
      error = thread_error;
      info = thread_info;
      debug = thread_debug;
    }




    void Exec::revert () {
      pthread_attr_destroy (default_attributes);
      delete default_attributes;
      default_attributes = NULL;

      print = previous_print;
      error = previous_error;
      info = previous_info;
      debug = previous_debug;
    }

    size_t available_cores () { 
      static const size_t number_of_threads = File::Config::get_int ("NumberOfThreads", 1);
      return (number_of_threads); 
    }

  }
}

