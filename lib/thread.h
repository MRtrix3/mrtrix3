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

#ifndef __mrtrix_thread_h__
#define __mrtrix_thread_h__

#include <pthread.h>
#include <map>

#include "exception.h"
#include "file/config.h"

namespace MR {
  namespace Thread {

    //! Launch & manage new threads
    /*! This class facilitates launching new threads, and waiting for them to
     * complete. A new thread will be started by executing the execute() method
     * of the object supplied to the start() member function. It is possible to
     * wait until a specific thread has finished executing by calling the
     * wait() method. The finish() method can be used to wait until all
     * currently running threads have completed.
     */
    class Launcher {
      private:
        class Instance {
          public:
            Instance (int new_ID, const std::string& identifier, const pthread_attr_t* attr, void *(*start_routine)(void*), void* data) : 
              name (identifier), ID (new_ID) { 
                debug ("launching thread \"" + name + "\" [ID " + str(ID) + "]..."); 
                pthread_t* tmp_thread = new pthread_t;
                if (pthread_create (tmp_thread, attr, start_routine, data)) {
                  delete tmp_thread;
                  throw Exception (std::string("error starting thread: ") + strerror (errno));
                }
                thread = tmp_thread;
              }
            ~Instance () { 
              if (thread) {
                debug ("waiting for completion of thread \"" + name + "\" [ID " + str(ID) + "]...");
                void* status;
                if (pthread_join (*thread, &status)) 
                  throw Exception (std::string("error joining thread: ") + strerror (errno));
              }
            }

            RefPtr<pthread_t> thread;
            const std::string name;
            int ID;
        };


      public:

        Launcher () : num_cores (File::Config::get_int ("NumberOfThreads", 1)), last_ID (0) { 
          pthread_attr_init (&attr);
          pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
        }
        ~Launcher () {
          finish();
          pthread_attr_destroy (&attr);
        }

        //! the number of cores to use for multi-threading, as specified in the MRtrix configuration file
        const int num_cores;

        //! launch a new thread, by invoking the execute() method of \a func
        /*! a new thread will be started by executing the execute() member
         * function of the object \a func of type \a F, which should have
         * an execute() member function with the following prototype:
         * \code
         * class MyFunc {
         *   public:
         *     void execute ();
         * };
         * \endcode
         *
         * A human-readable \a identifier should also be provided for debugging
         * and reporting purposes. 
         * \return a unique handle to the newly launched thread, which can be
         * passed to the wait() method.  */
        template <class F> int start (F& func, const std::string& identifier) {
          ++last_ID;
          threads[last_ID] = Instance (last_ID, identifier, &attr, static_exec<F>, static_cast<void*> (&func));
          return (last_ID);
        }

        //! wait for the thread with the specified ID to complete
        void wait (int ID) {
          std::map<int,Instance>::iterator it = threads.find (ID);
          if (it == threads.end()) 
            throw Exception ("unknown thread ID (" + str(ID) + ")");
          threads.erase (it);
        }

        //! wait for all currently running threads to complete
        void finish () { 
          if (threads.size()) {
            debug ("waiting for completion of all remaining threads...");
            threads.clear(); 
          }
        }

      private:
        pthread_attr_t attr;
        std::map<int, Instance> threads;
        int last_ID;

        template <class F> static void* static_exec (void* data) {
          F* func = static_cast<F*> (data);
          func->execute ();
          return (NULL);
        }
    };


    class Cond;

    //! Mutual exclusion lock
    /*! Used to protect critical sections of code from concurrent read & write
     * operations. The mutex should be locked using the lock() method prior to
     * modifying or reading the critical data, and unlocked using the unlock()
     * method as soon as the operation is complete.
     *
     * It is probably safer to use the member class Mutex::Lock, which helps to
     * ensure that the mutex is always released. The mutex will be locked as
     * soon as the Lock object is created, and released as soon as it goes out
     * of scope. This is especially useful in cases when there are several
     * exit points for the mutually exclusive code, since the compiler will
     * then always ensure the mutex is released. For example:
     * \code
     * Mutex mutex;
     * ...
     * void update () {
     *   Mutex::Lock lock (mutex);
     *   ...
     *   if (check_something()) {
     *     // lock goes out of scope when function returns,
     *     // The Lock destructor will then ensure the mutex is released.
     *     return;
     *   }
     *   ...
     *   // perform the update
     *   ...
     *   if (something_bad_happened) {
     *     // lock also goes out of scope when an exception is thrown,
     *     // ensuring the mutex is also released in these cases.
     *     throw Exception ("oops");
     *   }
     *   ...
     * } // mutex is released as lock goes out of scope
     * \endcode
     */
    class Mutex {
      public:
        Mutex () { pthread_mutex_init (&_mutex, NULL); }
        ~Mutex () { pthread_mutex_destroy (&_mutex); }

        void lock () { pthread_mutex_lock (&_mutex); }
        void unlock () { pthread_mutex_unlock (&_mutex); }

        class Lock {
          public:
            Lock (Mutex& mutex) : m (mutex) { m.lock(); }
            ~Lock () { m.unlock(); }
          private:
            Mutex& m;
        };
      private:
        pthread_mutex_t _mutex;
        friend class Cond;
    };


    //! Synchronise threads by waiting on a condition
    /*! This class allows threads to wait until a specific condition is
     * fulfilled, at which point the thread responsible for reaching that
     * condition will signal that the other waiting threads can be woken up.
     * These are used in conjunction with a Thread::Mutex object to protect the
     * relevant data. For example, the waiting thread may be executing:
     * \code
     * Mutex mutex;
     * Cond cond (mutex);
     * ...
     * void process_data () {
     *   while (true) {
     *     { // Mutex must be locked prior to waiting on condition
     *       Mutex::Lock lock (mutex);
     *       while (no_data()) cond.wait();
     *       get_data();
     *     } // Mutex is released as soon as possible
     *     ...
     *     // process data
     *     ...
     *   }
     * }
     * \endcode
     * While the other thread may be executing:
     * \code
     * void produce_data () {
     *   while (true) {
     *     ...
     *     // generate next batch of data
     *     ...
     *     { // Mutex must be locked prior to sending signal
     *       Mutex::Lock lock (mutex);
     *       submit_data();
     *       cond.signal();
     *     } // Mutex must be released for other threads to run
     *   }
     * }
     * \endcode 
     */
    class Cond {
      public:
        Cond (Mutex& mutex) : _mutex (mutex) { pthread_cond_init (&_cond, NULL); }
        ~Cond () { pthread_cond_destroy (&_cond); }

        //! wait until condition is reached
        void wait () { pthread_cond_wait (&_cond, &_mutex._mutex); }
        //! condition is reached: wake up at least one waiting thread
        void signal () { pthread_cond_signal (&_cond); }
        //! condition is reached: wake up all waiting threads
        void broadcast () { pthread_cond_broadcast (&_cond); }

      private:
        pthread_cond_t _cond;
        Mutex& _mutex;
    };



    template <class F, class T> class Serial {
      public:
        Serial (Launcher& launcher, F& func) : launch (launcher), functor (func) { }
        void start () { }
        void add (T& item) { }
      private:
        Launcher& launch;
        F& functor;
    };

    template <class F, class T> class Parallel {
      public:
        Parallel (Launcher& launcher, F& func) : launch (launcher), functor (func), num_threads (launcher.num_cores) { }

        void set_number_of_threads (int number_of_threads) { num_threads = number_of_threads; }
        void start () { }
        void add (T& item) { }
      private:
        Launcher& launch;
        F& functor;
        int num_threads;
    };

  }
}

#endif

