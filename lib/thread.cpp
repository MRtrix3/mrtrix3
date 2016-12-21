/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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
    //CONF Set the default number of CPU threads to use for multi-threading.

    size_t number_of_threads ()
    {
      if (__number_of_threads)
        return __number_of_threads;
      auto opt = App::get_options ("nthreads");
      if (opt.size()) {
        __number_of_threads = opt[0][0];
        return __number_of_threads;
      }

      const char* from_env = getenv ("MRTRIX_NTHREADS");
      if (from_env) {
        __number_of_threads = to<size_t> (from_env);
        return __number_of_threads;
      }

      __number_of_threads = std::thread::hardware_concurrency();
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

