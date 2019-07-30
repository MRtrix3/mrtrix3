/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <thread>
#include <atomic>

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
      nthreads_t __nthreads_type = nthreads_t::UNINITIALISED;

    }

    //CONF option: NumberOfThreads
    //CONF default: number of threads provided by hardware
    //CONF Set the default number of CPU threads to use for multi-threading.

    size_t number_of_threads ()
    {
      if (__nthreads_type != nthreads_t::UNINITIALISED)
        return __number_of_threads;

      auto opt = App::get_options ("nthreads");
      if (opt.size()) {
        __number_of_threads = opt[0][0];
        __nthreads_type = nthreads_t::EXPLICIT;
        return __number_of_threads;
      }

      const char* from_env = getenv ("MRTRIX_NTHREADS");
      if (from_env) {
        __number_of_threads = to<size_t> (from_env);
        __nthreads_type = nthreads_t::EXPLICIT;
        return __number_of_threads;
      }

      if (File::Config::get ("NumberOfThreads").size()) {
        const int i = File::Config::get_int ("NumberOfThreads", -1);
        if (i >= 0) {
          __number_of_threads = i;
          __nthreads_type = nthreads_t::EXPLICIT;
          return __number_of_threads;
        }
      }
      __number_of_threads = std::thread::hardware_concurrency();
      __nthreads_type = nthreads_t::IMPLICIT;
      return __number_of_threads;
    }


    nthreads_t type_nthreads()
    {
      return __nthreads_type;
    }





    void (*__Backend::previous_print_func) (const std::string& msg) = nullptr;
    void (*__Backend::previous_report_to_user_func) (const std::string& msg, int type) = nullptr;

    __Backend::__Backend () :
      refcount (0) {
        DEBUG ("initialising threads...");

        std::atomic<uint8_t> a;
        if (!a.is_lock_free())
          WARN ("atomic operations on uint8_t are not lock-free - this may introduce instabilities in operation!");

        if (sizeof (a) != sizeof (uint8_t))
          WARN ("size of atomic<uint8_t> differs from that of uint8_t - this may introduce instabilities in operation!");

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

