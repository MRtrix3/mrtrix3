/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 14/10/09.

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

#ifndef __mrtrix_thread_h__
#define __mrtrix_thread_h__

#include <thread>
#include <mutex>

#include "debug.h"
#include "exception.h"

/** \defgroup Thread Multi-threading
 * \brief functions to provide support for multi-threading
 *
 * These functions and class provide a simple interface for multi-threading in
 * MRtrix applications. Most of the low-level funtionality is a thin wrapper on
 * top to POSIX threads. There are two classes that are MRtrix-specific: 
 * \ref thread_queue, and \ref image_thread_looping. These two APIs provide
 * simple and convenient ways of multi-threading, and should be sufficient for
 * the vast majority of applications.
 *
 * Please refer to the \ref multithreading page for an overview of
 * multi-threading in MRtrix.
 *
 * \sa Image::ThreadedLoop
 * \sa thread_run_queue
 */

namespace MR
{
  namespace Thread
  {

    class __Backend {
      public:
        __Backend();
        ~__Backend();

        size_t refcount;

        std::mutex mutex;
        static void thread_print_func (const std::string& msg);
        static void thread_report_to_user_func (const std::string& msg, int type);

        static void (*previous_print_func) (const std::string& msg);
        static void (*previous_report_to_user_func) (const std::string& msg, int type);
    };

    extern __Backend* __backend;

    namespace {

      class __thread_base {
        public:
          __thread_base (const std::string& name = "unnamed") : name (name) { init(); }
          ~__thread_base () { done(); }


        protected:
          const std::string name;

          void init() {
            if (!__backend)
              __backend = new __Backend;
            ++__backend->refcount;
          }
          void done() {
            --__backend->refcount;
            if (!__backend->refcount) {
              delete __backend;
              __backend = nullptr;
            }
          }
      };


      class __single_thread : public __thread_base {
        public:
          template <class Functor>
            __single_thread (Functor&& functor, const std::string& name = "unnamed") :
            __thread_base (name) { 
              DEBUG ("launching thread \"" + name + "\"...");
              typedef typename std::remove_reference<Functor>::type F;
              thread = std::thread (&F::execute, std::ref (functor));
            }
          __single_thread (const __single_thread& s) = delete;
          __single_thread (__single_thread&& s) = default;

          ~__single_thread () { 
            DEBUG ("waiting for completion of thread \"" + name + "\"...");
            thread.join(); 
            DEBUG ("thread \"" + name + "\" completed OK");
          }

        protected:
          std::thread thread;
      };


      template <class Functor>
        class __multi_thread : public __thread_base {
          public:
            __multi_thread (Functor& functor, size_t nthreads, const std::string& name = "unnamed") :
              __thread_base (name), functors (nthreads-1, functor) { 
                DEBUG ("launching " + str (nthreads) + " threads \"" + name + "\"...");
                typedef typename std::remove_reference<Functor>::type F;
                threads.reserve (nthreads);
                for (auto& f : functors) 
                  threads.push_back (std::thread (&F::execute, std::ref (f)));
                threads.push_back (std::thread (&F::execute, std::ref (functor)));
              }

            __multi_thread (const __multi_thread& m) = delete;
            __multi_thread (__multi_thread&& m) = default;

            ~__multi_thread () { 
              DEBUG ("waiting for completion of threads \"" + name + "\"...");
              for (auto& t : threads) 
                t.join();
              DEBUG ("threads \"" + name + "\" completed OK");
            }
          protected:
            std::vector<std::thread> threads;
            std::vector<typename std::remove_reference<Functor>::type> functors;

        };


      template <class Functor> 
        class __Multi {
          public:
            __Multi (Functor& object, size_t number) : functor (object), num (number) { }
            __Multi (__Multi&& m) = default;
            template <class X> bool operator() (const X&) { assert (0); return false; }
            template <class X> bool operator() (X&) { assert (0); return false; }
            template <class X, class Y> bool operator() (const X&, Y&) { assert (0); return false; }
            typename std::remove_reference<Functor>::type& functor;
            size_t num;
        };

      template <class Functor>
        class __run {
          public:
            typedef __single_thread type;
            type operator() (Functor& functor, const std::string& name) {
              return { functor, name };
            }
        };

      template <class Functor>
        class __run<__Multi<Functor>> {
          public:
            typedef __multi_thread<Functor> type;
            type operator() (__Multi<Functor>& functor, const std::string& name) {
              return { functor.functor, functor.num, name };
            }
        };

    }



    /** \addtogroup Thread
     * @{ */

    /** \defgroup thread_basics Basic multi-threading primitives
     * \brief basic functions and classes to allow multi-threading
     *
     * These functions and classes mostly provide a thin wrapper around the
     * POSIX threads API. While they can be used as-is to develop
     * multi-threaded applications, in practice the \ref image_thread_looping
     * and \ref thread_queue APIs provide much more convenient and powerful
     * ways of developing robust and efficient applications.
     * 
     * @{ */

    /*! the number of cores to use for multi-threading, as specified in the
     * variable NumberOfThreads in the MRtrix configuration file, or set using
     * the -nthreads command-line option */
    size_t number_of_threads ();



    //! used to request multiple threads of the corresponding functor
    /*! This function is used in combination with Thread::run or
     * Thread::run_queue to request that the functor \a object be run in
     * parallel using \a number threads of execution (defaults to
     * Thread::number_of_threads()). 
     * \sa Thread::run_queue() */
    template <class Functor>
      inline __Multi<typename std::remove_reference<Functor>::type> 
      multi (Functor&& functor, size_t nthreads = number_of_threads()) 
      {
        return { functor, nthreads };
      }



    //! Execute the functor's execute method in a separate thread
    /*! Launch a thread by running the execute method of the object \a functor,
     * which should have the following prototype:
     * \code
     * class MyFunc {
     *   public:
     *     void execute ();
     * };
     * \endcode
     *
     * The thread is launched by the constructor, and the destructor will wait
     * for the thread to finish.  The lifetime of a thread launched via this
     * method is therefore restricted to the scope of the returned object. For
     * example:
     * \code
     * class MyFunctor {
     *   public:
     *     void execute () {
     *       ...
     *       // do something useful
     *       ...
     *     }
     * };
     *
     * void some_function () {
     *   MyFunctor func; // parameters can be passed to func in its constructor
     *
     *   // thread is launched as soon as my_thread is instantiated:
     *   auto my_thread = Thread::run (func, "my function");
     *   ...
     *   // do something else while my_thread is running
     *   ...
     * } // my_thread goes out of scope: current thread will halt until my_thread has completed
     * \endcode
     *
     * It is also possible to launch an array of threads in parallel, by
     * wrapping the functor into a call to Thread::multi(), as follows:
     * \code
     * ...
     * auto my_threads = Thread::run (Thread::multi (func), "my function");
     * ...
     * \endcode
     */
    template <class Functor>
      inline typename __run<Functor>::type run (Functor&& functor, const std::string& name = "unnamed") 
      {
        return __run<typename std::remove_reference<Functor>::type>() (functor, name); 
      }

    /** @} */
    /** @} */
  }
}

#endif


