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

#include "exception.h"
#include "file/config.h"

namespace MR {

  class Thread 
  {
    private:
      struct Info {
        Thread* instance;
        int number;
      };  
      static void* static_exec (void* __info) { 
        static_cast<Info*>(__info)->instance->execute (static_cast<Info*>(__info)->number);
        return (NULL); 
      }

    protected:
      int num_threads;
      virtual void execute (int thread_number) = 0;

    public:
      Thread () : num_threads (File::Config::get_int ("NumberOfThreads", 1)) { } 
      virtual ~Thread () { }

      void set_number_of_threads (int number_of_threads = 0) { num_threads = number_of_threads ? number_of_threads : File::Config::get_int ("NumberOfThreads", 1); }

      void run ()  
      { 
        info ("launching " + str (num_threads) + " threads");
        pthread_attr_t __attr;
        pthread_attr_init (&__attr);
        pthread_attr_setdetachstate (&__attr, PTHREAD_CREATE_JOINABLE);

        pthread_t __threads [num_threads-1];
        struct Info __info [num_threads-1];

        for (int i = 0; i < num_threads-1; i++) {
          __info[i].instance = this;
          __info[i].number = i+1;
          if (pthread_create (&__threads[i], &__attr, Thread::static_exec, (void*) &__info[i])) 
            throw Exception (std::string("error starting thread: ") + strerror (errno));
        }

        execute (0);

        void* __status;
        for (int i = 0; i < num_threads-1; i++) 
          if (pthread_join (__threads[i], &__status)) 
            throw Exception (std::string("error joining thread: ") + strerror (errno));

        pthread_attr_destroy (&__attr);
      }   


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

  };
}

#endif

