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

#ifndef __mrtrix_thread_exec_h__
#define __mrtrix_thread_exec_h__

#include "exception.h"
#include "file/config.h"
#include "thread/mutex.h"

/** \defgroup Thread Multi-threading
 * \brief functions to provide support for multi-threading 
 *
 * These functions and class provide a simple interface for multi-threading in
 * MRtrix applications. Most applications will probably find that the
 * Thread::Queue and Thread::Exec classes are sufficient for their needs. 
 *
 * \note The Thread::init() functin must be called \e before using any of the
 * multi-threading functionality described here.
 */

namespace MR {
  namespace Thread {

    /** \addtogroup Thread 
     * @{ */

    /*! the number of cores to use for multi-threading, as specified in the
     * variable NumberOfThreads in the MRtrix configuration file */
    size_t available_cores ();

    //! Create an array of duplicate functors to execute in parallel
    /*! Use this class to hold an array of duplicates of the supplied functor,
     * so that they can be executed in parallel using the Thread::Exec class.
     * Note that the original functor will be used, and as many copies will be
     * created as needed to make up a total of \a num_threads. By default, \a
     * num_threads is given by Thread::available_cores(). 
     *
     * \note Each Functor copy will be created using its copy constructor using
     * the original \a functor as the original. It is essential therefore that
     * the copy consructor behave as expected. 
     *
     * For example:
     * \code
     * void my_function () {
     *   // Create master copy of functor:
     *   MyThread my_thread (param);
     *
     *   // Duplicate as needed up to a maximum of Thread::available_cores() threads.
     *   // Each copy is created using the copy constructor:
     *   Thread::Array<MyThread> list (my_thread); 
     *
     *   // Launch all copies in parallel:
     *   Thread::Exec threads (list, "my threads");
     * } // all copies in the array are deleted when the array goes out of scope.
     * \endcode
     */
    template <class Functor> class Array {
      public: 
        Array (Functor& functor, size_t num_threads = available_cores()) :
          first_functor (functor), functors (num_threads-1) {
            assert (num_threads);
            for (size_t i = 0; i < num_threads-1; i++) 
              functors[i] = new Functor (functor);
          }
      private:
        Functor& first_functor;
        VecPtr<Functor> functors;
        friend class Exec;
    };



    //! Execute the functor's execute method in a separate thread
    /*! Lauch a thread by running the execute method of the object \a functor,
     * which should have the following prototype:
     * \code
     * class MyFunc {
     *   public:
     *     void execute ();
     * };
     * \endcode
     *
     * It is also possible to launch an array of threads in parallel. See
     * Thread::Array for details
     *
     * The thread is launched by the constructor, and the destructor will wait
     * for the thread to finish.  The lifetime of a thread launched via this
     * method is therefore restricted to the scope of the Exec object. For
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
     *   Thread::Exec my_thread (func, "my function");
     *   ...
     *   // do something else while my_thread is running
     *   ...
     * } // my_thread goes out of scope: current thread will halt until my_thread has completed
     * \endcode
     */
    class Exec {
      public:
        //! Start a new thread that will execute the execute() method of \a functor
        /*! A human-readable identifier can be supplied via the \a description
         * parameter. This is helping for debugging and error reporting. */
        template <class Functor> Exec (Functor& functor, const std::string& description = "unnamed") : 
          ID (1), name (description), responsible (false) { 
            if (!default_attributes) init();
            info ("launching thread \"" + name + "\"");
            start (ID[0], functor); 
          }

        //! Start an array of new threads each runnning the execute() method of its \a functor
        /*! A human-readable identifier can be supplied via the \a description
         * parameter. This is helping for debugging and error reporting. */
        template <class Functor> Exec (Array<Functor>& functor, const std::string& description = "unnamed") : 
          ID (functor.functors.size()+1), name (description), responsible (false) {
            if (!default_attributes) init();
            info ("launching " + str(ID.size()) + " thread" + (ID.size() > 1 ? "s" : "") +  " \"" + name + "\"");
            start (ID[0], functor.first_functor);
            for (size_t i = 1; i < ID.size(); ++i) 
              start (ID[i], *functor.functors[i-1]);
          }

        //! Wait for the thread to terminate
        /*! The thread will terminate when the execute() method of the \a
         * functor object returns. */
        ~Exec () { 
          for (size_t i = 0; i < ID.size(); ++i) {
            debug ("waiting for completion of thread \"" + name + "\" [ID " + str(ID[i]) + "]...");
            void* status;
            if (pthread_join (ID[i], &status)) 
              throw Exception (std::string("error joining thread \"" + name + "\" [ID " + str(ID[i]) + "]: ") + strerror (errno));
            debug ("thread \"" + name + "\" [ID " + str(ID[i]) + "] completed OK");
          }

          if (responsible) revert();
        }
        
      private:
        std::vector<pthread_t> ID;
        std::string name;
        bool responsible;

        template <class Functor> void start (pthread_t& id, Functor& functor) { 
          if (pthread_create (&id, default_attributes, static_exec<Functor>, static_cast<void*> (&functor))) 
            throw Exception (std::string("error launching thread \"" + name + "\": ") + strerror (errno));
          debug ("launched thread \"" + name + "\" [ID " + str(id) + "]..."); 
        }

        template <class Functor> static void* static_exec (void* data) { 
          try { static_cast<Functor*>(data)->execute (); }
          catch (Exception& E) { E.display(); }
          return (NULL); 
        }

        void init ();
        void revert ();

        static pthread_attr_t* default_attributes;
        static pthread_t main_thread;

        static Mutex mutex;
        static void thread_print (const std::string& msg);
        static void thread_error (const std::string& msg);
        static void thread_info  (const std::string& msg);
        static void thread_debug (const std::string& msg);

        static void (*previous_print) (const std::string& msg);
        static void (*previous_error) (const std::string& msg);
        static void (*previous_info)  (const std::string& msg);
        static void (*previous_debug) (const std::string& msg);
    };

    /** @} */
  }
}

#endif


