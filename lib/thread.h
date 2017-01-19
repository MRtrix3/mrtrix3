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

#ifndef __mrtrix_thread_h__
#define __mrtrix_thread_h__

#include <thread>
#include <future>
#include <mutex>

#include "debug.h"
#include "mrtrix.h"
#include "exception.h"

/** \defgroup thread_classes Multi-threading
 * \brief functions to provide support for multi-threading
 *
 * These functions and associated classes provide a simple interface for
 * multi-threading in MRtrix applications. Most of the low-level funtionality
 * relies on the C++11 `std::thread` API. MRtrix3 builds on this to add three
 * convenience methods: 
 * 
 * - [Thread::run()](@ref Thread::run()) to launch one or more worker threads;
 *
 * - [ThreadedLoop()](@ref image_thread_looping) to run an operation over all voxels
 *   in one or more images;
 *
 * - [Thread::run_queue()](@ref Thread::run_queue()) to run a pipeline,
 *   with one or more threads feeding data through to one or more other threads
 *   (potentially with further stages in the pipeline).
 *
 * These APIs provide simple and convenient ways of multi-threading, and should
 * be sufficient for the vast majority of applications.
 *
 * Please refer to the \ref multithreading page for an overview of
 * multi-threading in MRtrix.
 *
 * \sa Thread::run()
 * \sa threaded_loop
 * \sa Thread::run_queue()
 */

namespace MR
{
  namespace Thread
  {

    class __Backend { NOMEMALIGN
      public:
        __Backend();
        ~__Backend();

        static void register_thread () { 
          std::lock_guard<std::mutex> lock (mutex);
          if (!backend)
            backend = new __Backend;
          ++backend->refcount; 
        }
        static void unregister_thread () {
          assert (backend);
          std::lock_guard<std::mutex> lock (mutex);
          if (!(--backend->refcount)) {
            delete backend;
            backend = nullptr;
          }
        }

        static void thread_print_func (const std::string& msg);
        static void thread_report_to_user_func (const std::string& msg, int type);

        static void (*previous_print_func) (const std::string& msg);
        static void (*previous_report_to_user_func) (const std::string& msg, int type);

      protected:
        size_t refcount;

        static __Backend* backend;
        static std::mutex mutex;
    };


    namespace {

      class __thread_base { NOMEMALIGN
        public:
          __thread_base (const std::string& name = "unnamed") : name (name) { __Backend::register_thread(); }
          __thread_base (const __thread_base&) = delete;
          __thread_base (__thread_base&&) = default;
          ~__thread_base () { __Backend::unregister_thread(); }


        protected:
          const std::string name;
      };


      class __single_thread : public __thread_base { NOMEMALIGN
        public:
          template <class Functor>
            __single_thread (Functor&& functor, const std::string& name = "unnamed") :
            __thread_base (name) { 
              DEBUG ("launching thread \"" + name + "\"...");
              typedef typename std::remove_reference<Functor>::type F;
              thread = std::async (std::launch::async, &F::execute, &functor);
            }
          __single_thread (const __single_thread&) = delete;
          __single_thread (__single_thread&&) = default;

          void wait () noexcept (false) {
            DEBUG ("waiting for completion of thread \"" + name + "\"...");
            thread.get(); 
            DEBUG ("thread \"" + name + "\" completed OK");
          }

          ~__single_thread () {
            if (thread.valid()) {
              try { wait(); }
              catch (Exception& E) { E.display(); }
            }
          }

        protected:
          std::future<void> thread;
      };


      template <class Functor>
        class __multi_thread : public __thread_base { NOMEMALIGN
          public:
            __multi_thread (Functor& functor, size_t nthreads, const std::string& name = "unnamed") :
              __thread_base (name), functors ( (nthreads>0 ? nthreads-1 : 0), functor) { 
                DEBUG ("launching " + str (nthreads) + " threads \"" + name + "\"...");
                typedef typename std::remove_reference<Functor>::type F;
                threads.reserve (nthreads);
                for (auto& f : functors) 
                  threads.push_back (std::async (std::launch::async, &F::execute, &f));
                threads.push_back (std::async (std::launch::async, &F::execute, &functor));
              }

            __multi_thread (const __multi_thread&) = delete;
            __multi_thread (__multi_thread&&) = default;

            void wait () noexcept (false) {
              DEBUG ("waiting for completion of threads \"" + name + "\"...");
              bool exception_thrown = false;
              for (auto& t : threads) {
                if (!t.valid()) 
                  continue;
                try { t.get(); }
                catch (Exception& E) { 
                  exception_thrown = true;
                  E.display(); 
                }
              }
              if (exception_thrown) 
                throw Exception ("exception thrown from one or more threads \"" + name + "\"");
              DEBUG ("threads \"" + name + "\" completed OK");
            }

            bool any_valid () const {
              for (auto& t : threads) 
                if (t.valid()) 
                  return true;
              return false;
            }

            ~__multi_thread () {
              if (any_valid()) {
                try { wait(); }
                catch (Exception& E) { E.display(); }
              }
            }
          protected:
            vector<std::future<void>> threads;
            vector<typename std::remove_reference<Functor>::type> functors;

        };


      template <class Functor> 
        class __Multi { NOMEMALIGN
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
        class __run { NOMEMALIGN
          public:
            typedef __single_thread type;
            type operator() (Functor& functor, const std::string& name) {
              return { functor, name };
            }
        };

      template <class Functor>
        class __run<__Multi<Functor>> { NOMEMALIGN
          public:
            typedef __multi_thread<Functor> type;
            type operator() (__Multi<Functor>& functor, const std::string& name) {
              return { functor.functor, functor.num, name };
            }
        };

    }



    /** \addtogroup thread_classes
     * @{ */

    /** \defgroup thread_basics Basic multi-threading primitives
     * \brief basic functions and classes to allow multi-threading
     *
     * These functions and classes mostly provide a thin wrapper around the
     * C++11 threads API. While they can be used as-is to develop
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
     * \sa Thread::run() 
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
     *
     * \par Exception handling
     *
     * Proper handling of exceptions in a multi-threaded context is
     * non-trivial, and in general you should take every precaution to prevent
     * threads from throwing exceptions. This means you should perform all
     * error checking within a single-threaded context, before starting
     * processing-intensive threads, so as to minimise the chances of anything
     * going wrong at that stage. 
     *
     * In this implementation, the wait() function can be used to wait until
     * all threads have completed, at which point any exceptions thrown will be
     * displayed, and a futher exception re-thrown to allow the main
     * application to catch the error (this could be the same exception that
     * was originally thrown if a single thread was run). This means the
     * application will continue processing if any of the remaining threads
     * remain active, and it may be a while before the application itself is
     * allowed to handle the error appropriately. If this is behaviour is not
     * appropriate, and you expect exceptions to be thrown occasionally, you
     * should take steps to handle these yourself (e.g. by setting / checking
     * some flag within your threads, etc.).
     *
     * \note while the wait() function will also be invoked in the destructor,
     * any exceptions thrown will be caught and \e not re-thrown (throwing in
     * the destructor is considered bad practice). This is to prevent undefined
     * behaviour (i.e. crashes) when multiple thread objects are launched
     * within the same scope, each of which might throw. In these cases, it is
     * best to explicitly call wait() for each of the objects returned by
     * Thread::run(), rather than relying on the destructor alone (note
     * Thread::Queue already does this). 
     *
     * \sa Thread::multi()
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


