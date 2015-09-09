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

#include <thread>

#include "app.h"
#include "thread.h"
#include "file/config.h"
#include "thread_queue.h"

namespace MR
{
  namespace Thread
  {

    namespace {

      size_t __number_of_threads = 0;

    }

    //CONF option: NumberOfThreads
    //CONF default: number of threads provided by hardware
    //CONF set the default number of CPU threads to use for multi-threading.

    size_t number_of_threads ()
    {
      if (__number_of_threads)
        return __number_of_threads;
      const App::Options opt = App::get_options ("nthreads");
      __number_of_threads = opt.size() ? opt[0][0] : File::Config::get_int ("NumberOfThreads", std::thread::hardware_concurrency());
      return __number_of_threads;
    }





    void (*__Backend::previous_print_func) (const std::string& msg) = nullptr;
    void (*__Backend::previous_report_to_user_func) (const std::string& msg, int type) = nullptr;

    __Backend::__Backend () :
      refcount (0) {
        DEBUG ("initialising threads...");

        previous_print_func = print;
        previous_report_to_user_func = report_to_user_func;

        print = thread_print_func;
        report_to_user_func = thread_report_to_user_func;
      }

    __Backend::~__Backend () 
    {
      print = previous_print_func;
      report_to_user_func = previous_report_to_user_func;
    }

    void __Backend::thread_print_func (const std::string& msg)
    {
      std::lock_guard<std::mutex> lock (mutex);
      previous_print_func (msg);
    }

    void __Backend::thread_report_to_user_func (const std::string& msg, int type)
    {
      std::lock_guard<std::mutex> lock (mutex);
      previous_report_to_user_func (msg, type);
    }


    __Backend* __Backend::backend = nullptr;
    std::mutex __Backend::mutex;

  }
}

