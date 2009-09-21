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

    class Launcher {
      private:
        class Instance {
          public:
            Instance (int new_ID, const std::string& identifier) : functor (NULL), name (identifier), ID (new_ID) { 
              debug ("launching thread \"" + name + "\" [ID " + str(ID) + "]..."); }
            ~Instance () { if (functor) join (); }
            void join () {
              debug ("waiting for completion of thread \"" + name + "\" [ID " + str(ID) + "]...");
              void* status;
              if (pthread_join (thread, &status)) 
                throw Exception (std::string("error joining thread: ") + strerror (errno));
            }

            pthread_t thread;
            void* functor;
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
         * function of the object \a func. A human-readable identifier should
         * also be provided for debugging and reporting purposes. 
         * \return a unique handle to the newly launched thread, which can be
         * passed to the wait() method.  */
        template <class F> int start (F& func, const std::string& identifier) {
          ++last_ID;
          std::pair<const int,Instance>* p = &(*threads.insert (
                std::pair<int,Instance> (last_ID, Instance(last_ID, identifier))).first);
          assert(p->second.functor == NULL);
          p->second.functor = static_cast<void*> (&func);
          if (pthread_create (&p->second.thread, &attr, static_exec<F>, static_cast<void*> (p))) 
            throw Exception (std::string("error starting thread: ") + strerror (errno));
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
          std::pair<int,Instance>* p = static_cast<std::pair<int,Instance>*> (data);
          F* func = static_cast<F*> (p->second.functor);
          func->execute (p->first);
          return (NULL);
        }
    };


    class Cond;

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


    class Cond {
      public:
        Cond () { pthread_cond_init (&_cond, NULL); }
        ~Cond () { pthread_cond_destroy (&_cond); }

        void wait (Mutex& mutex) { pthread_cond_wait (&_cond, &mutex._mutex); }
        void signal () { pthread_cond_signal (&_cond); }
        void broadcast () { pthread_cond_broadcast (&_cond); }

      private:
        pthread_cond_t _cond;
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

