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
#include <vector>
#include <queue>

#include "exception.h"
#include "file/config.h"

/** \defgroup Thread Multi-threading
 * \brief functions to provide support for multi-threading */

namespace MR {
  namespace Thread {

    //! \cond skip
    pthread_attr_t* const default_attributes ();
    //! \endcond
    
    /** \addtogroup Thread 
     * @{ */

    //! the number of cores to use for multi-threading, as specified in the MRtrix configuration file
    size_t num_cores (); 



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
     * While the thread producing the data may be executing:
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





    //! the thread unique identifier
    class Identifier {
      public:
        Identifier () { }
        Identifier (const Identifier& ID) : pthread_ID (ID.pthread_ID) { }
        Identifier (const pthread_t ID) : pthread_ID (ID) { }
        Identifier operator= (const Identifier& ID) { pthread_ID = ID.pthread_ID; return (*this); }
        Identifier operator= (const pthread_t ID) { pthread_ID = ID; return (*this); }

        bool operator== (Identifier ID) { return (pthread_equal (pthread_ID, ID.pthread_ID)); }
        bool operator!= (Identifier ID) { return (!pthread_equal (pthread_ID, ID.pthread_ID)); }

        pthread_t pthread_ID;
    };
    std::ostream& operator<< (std::ostream& stream, const Identifier ID) { stream << ID.pthread_ID; return (stream); }

    //! returns the currently running thread's unique identifier
    Identifier ID () { return (pthread_self()); }


    //! represents an instance of a running thread
    /*! Lauch a thread by running the execute method of the object \a functor,
     * which should have the following prototype:
     * \code
     * class MyFunc {
     *   public:
     *     void execute ();
     * };
     * \endcode
     *
     * Supplying a sensible identifier in \a name is useful for debugging and
     * reporting purposes.
     * 
     * The thread is launched in the constructor, and the destructor will wait
     * for the thread to finish. The lifetime of a thread launched via this
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
        Exec (const std::string& description = "unnamed") : attr (default_attributes()), name (description), running (false) { }
        virtual ~Exec () { finish(); }

        void start () {
          if (pthread_create (&TID.pthread_ID, attr, static_exec, static_cast<void*> (this))) 
            throw Exception (std::string("error launching thread \"" + name + "\": ") + strerror (errno));
          running = true;
          debug ("launched thread \"" + name + "\" [ID " + str(TID) + "]..."); 
        }

        void finish () { 
          if (running) {
            debug ("waiting for completion of thread \"" + name + "\" [ID " + str(TID) + "]...");
            void* status;
            if (pthread_join (TID.pthread_ID, &status)) 
              throw Exception (std::string("error joining thread \"" + name + "\" [ID " + str(TID) + "]: ") + strerror (errno));
            running = false;
            debug ("thread \"" + name + "\" [ID " + str(TID) + "] completed OK");
          }
        }

        Identifier ID () const { return (TID); }

        //! this function must be defined in derived classes
        virtual void execute () = 0;

      protected:
        //! the thread attributes can be overriden in derived classes if needed
        pthread_attr_t* attr;

      private:
        Identifier TID;
        std::string name;
        bool running;

        static void* static_exec (void* data);
    };





    //! a first-in first-out thread-safe item queue
    /*! This class implements a means of pushing data items into a queue, so
     * that they can each be processed in one or more separate threads.
     * Items of type \a T are pushed onto the queue using the push() method,
     * and will be processed on a first-in, first-out basis. The push() method
     * will return \c false unless the queue has been closed, either following
     * a call to end(), or because the processing thread has completed.
     *
     * The processing itself is done in one or more separate threads by
     * invoking the process() method of the \a func object (of type \a F),
     * which should have the following prototype:
     * \code
     * class MyFunctor {
     *   public:
     *     bool process (T item);
     * };
     * \endcode
     * The process() method should return \c false while data items still need
     * to be processed. Returning \c true will cause the processing thread(s)
     * to terminate, and all subsequent calls to push() to return \c true
     * without pushing any data onto the queue.
     *
     * Processing can also be halted by invoking the end() method, which will
     * prevent any further items from being pushed onto the queue. The
     * processing thread will continue until all items on the queue have been
     * flushed out (or it decides to terminate by returning \c true). 
     *
     * The set_buffer_size() method can be used to set the maximum number of
     * items that can be pushed onto the queue before blocking. If a thread
     * attempts to push more data onto the queue when the queue already
     * contains this number of items, the thread will block until at least one
     * item has been processed.
     */
    template <class T> class Queue {
      public:
        Queue (const std::string& description = "unnamed") : 
          more_data (mutex),
          more_space (mutex),
          capacity (100),
          pusher_count (0),
          popper_count (0),
          name (description) { }
        ~Queue () { while (fifo.size()) fifo.pop(); }

        void set_buffer_size (size_t max_size) { Mutex::Lock lock (mutex); capacity = max_size; }

        class Push {
          public: 
            Push (Queue<T>& queue) : Q (queue) { }
            void open () { Q.register_pusher(); }
            void close () { Q.unregister_pusher(); }
            bool push (T* item) { return (Q.push (item)); }
          private:
            Queue<T>& Q;
        };

        class Pop {
          public: 
            Pop (Queue<T>& queue) : Q (queue) { }
            void open () { Q.register_popper(); }
            void close () { Q.unregister_popper(); }
            T* pop () { return (Q.pop()); }
          private:
            Queue<T>& Q;
        };

        void status () { 
          Mutex::Lock lock (mutex);
          std::cerr << "Thread::Queue " + name + ":\n  producers: " << pusher_count << "\n  consumers: " << popper_count << "\n  items waiting: " << fifo.size() << "\n";
        }


      private:
        Mutex mutex;
        Cond more_data, more_space;
        std::queue<T*> fifo;
        size_t capacity;
        size_t pusher_count, popper_count;
        std::string name;

        void register_pusher ()   { Mutex::Lock lock (mutex); ++pusher_count; }
        void unregister_pusher () { 
          Mutex::Lock lock (mutex);
          assert (pusher_count); 
          --pusher_count; 
          if (!pusher_count) {
            debug ("no producers left on queue \"" + name + "\"");
            more_data.broadcast(); 
          }
        }
        void register_popper ()   { Mutex::Lock lock (mutex); ++popper_count; }
        void unregister_popper () { 
          Mutex::Lock lock (mutex); 
          assert (popper_count);
          --popper_count; 
          if (!popper_count) {
            debug ("no consumers left on queue \"" + name + "\"");
            more_space.broadcast(); 
          }
        }

        bool push (T* item) { 
          Mutex::Lock lock (mutex); 
          while (fifo.size() >= capacity && popper_count) more_space.wait();
          if (!popper_count) return (false);
          fifo.push (item);
          more_data.signal();
          return (true);
        }

        T* pop () {
          Mutex::Lock lock (mutex);
          while (fifo.empty() && pusher_count) more_data.wait();
          if (fifo.empty() && !pusher_count) return (NULL);
          T* item = fifo.front();
          fifo.pop();
          more_space.signal();
          return (item);
        }

        friend class Push;
        friend class Pop;
    };


    /** @} */
  }
}

#endif

