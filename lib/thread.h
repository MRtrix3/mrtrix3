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

    /** \addtogroup Thread 
     * @{ */

    bool initialised ();
    //! Initialise the thread system
    /*! This function must be called before using any of the functionality in
     * the multi-threading system. */
    void init ();

    //! the number of cores to use for multi-threading, as specified in the MRtrix configuration file
    size_t num_cores ();
    const pthread_attr_t* default_attributes();




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
     * Supplying a sensible identifier in \a name is useful for debugging and
     * reporting purposes.
     * 
     * The thread is launched using the start() method, or using the
     * non-default constructor. The destructor will wait for the thread to
     * finish, although the application can explicitly wait for the thread to
     * complete by calling its finish() method. The lifetime of a thread
     * launched via this method is therefore restricted to the scope of the
     * Exec object. For example:
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
        Exec () : running (false) { }
        template <class Functor> Exec (Functor& functor, const std::string& description = "unnamed") :
          running (false) { start (functor, description); }
        ~Exec () { finish(); }

        template <class Functor> void start (Functor& functor, const std::string& description = "unnamed") {
          finish();
          name = description;
          if (pthread_create (&ID, default_attributes(), static_exec<Functor>, static_cast<void*> (&functor))) 
            throw Exception (std::string("error launching thread \"" + name + "\": ") + strerror (errno));
          running = true;
          debug ("launched thread \"" + name + "\" [ID " + str(ID) + "]..."); 
        }

        void finish () { 
          if (running) {
            debug ("waiting for completion of thread \"" + name + "\" [ID " + str(ID) + "]...");
            void* status;
            if (pthread_join (ID, &status)) 
              throw Exception (std::string("error joining thread \"" + name + "\" [ID " + str(ID) + "]: ") + strerror (errno));
            running = false;
            debug ("thread \"" + name + "\" [ID " + str(ID) + "] completed OK");
          }
        }

      private:
        pthread_t ID;
        std::string name;
        bool running;

        template <class Functor> static void* static_exec (void* data) { static_cast<Functor*>(data)->execute (); return (NULL); }
    };





    //! A first-in first-out thread-safe item queue
    /*! This class implements a thread-safe means of pushing data items into a
     * queue, so that they can each be processed in one or more separate
     * threads. Pointers to items of type \a T are pushed onto the queue using
     * the membar class Thread::Queue<T>::Push, and will be processed on a
     * first-in, first-out basis. Pointers to these items are then retrieved
     * using the member class Thread::Queue<T>::Pop. 
     *
     * For example:
     * \code
     * class Item {
     *   public:
     *     ...
     *     // data members
     *     ...
     * };
     *
     * class Sender {
     *   public:
     *     Sender (Thread::Queue<Item>& queue) : push (queue) { } 
     *     void execute () {
     *       while (need_more_items()) {
     *         Item* item = new Item;
     *         ...
     *         // prepare item
     *         ...
     *         if (!push (item)) break; // break if push() returns false
     *       }
     *       push.close(); // this MUST be called before execute() returns
     *     }
     *   private:
     *     Thread::Queue<Item>::Push push;
     * };
     * 
     * class Receiver {
     *   public:
     *     Receiver (Thread::Queue<Item>& queue) : pop (queue) { } 
     *     void execute () {
     *       Item* item;
     *       while ((item = pop())) { // break when pop() returns NULL
     *         ...
     *         // process item
     *         ...
     *         delete item;
     *       }
     *       pop.close(); // this MUST be called before execute() returns
     *     }
     *   private:
     *     Thread::Queue<Item>::Pop pop;
     * };
     * 
     * void my_function () {
     *   Thread::Queue<Item> queue;
     *   Sender sender (queue);
     *   Receiver receiver (queue);
     *   
     *   Thread::Exec sender_thread (sender);
     *   Thread::Exec receiver_thread (receiver);
     * }
     * \endcode
     *
     * \note The Thread::Queue<T>::Push and/or Thread::Queue<T>::Pop object \e
     * must be instanciated \e before any of the threads accessing the queue
     * are launched. This is most conveniently done by defining them as members
     * of the functor class, and having them intialised in the constructor, as
    * in the example above.
     *
     * \note The Thread::Queue<T>::Push and/or Thread::Queue<T>::Pop close() method
     * \e must be called as soon as possible after completion of processing to
     * ensure that no deadlocks occur at exit. In particular, it should be
     * called before each thread's execute() method returns, \e not in each
     * functor object's destructor. 
     */
    template <class T> class Queue {
      public:
        //! Construct a Queue of items of type \c T
        /*! \param description a string identifying the queue for degugging purposes
         * \param buffer_size the maximum number of items that can be pushed onto the queue before
         * blocking. If a thread attempts to push more data onto the queue when the
         * queue already contains this number of items, the thread will block until
         * at least one item has been popped.  By default, the buffer size is 100
         * items.
         */
        Queue (const std::string& description = "unnamed", size_t buffer_size = 100) : 
          more_data (mutex),
          more_space (mutex),
          capacity (buffer_size),
          writer_count (0),
          reader_count (0),
          name (description) { }
        ~Queue () { while (fifo.size()) fifo.pop(); }

        //! This class is used to push items onto the queue
        /*! Items cannot be pushed directly onto a Thread::Queue<T> queue. An
         * object of this class must be instanciated and used to write to the
         * queue. This is done to ensure the number of writers to the queue can
         * be tracked. */
        class Push {
          public: 
            Push (Queue<T>& queue) : Q (queue) { Q.register_writer(); }
            void close () { Q.unregister_writer(); }
            bool operator() (T* item) { return (Q.push (item)); }
          private:
            Queue<T>& Q;
        };

        //! This class is used to pop items from the queue
        /*! Items cannot be popped directly from a Thread::Queue<T> queue. An
         * object of this class must be instanciated and used to read from the
         * queue. This is done to ensure the number of readers from the queue can
         * be tracked. */
        class Pop {
          public: 
            Pop (Queue<T>& queue) : Q (queue) { Q.register_reader(); }
            void close () { Q.unregister_reader(); }
            T* operator() () { return (Q.pop()); }
          private:
            Queue<T>& Q;
        };

        void status () { 
          Mutex::Lock lock (mutex);
          std::cerr << "Thread::Queue " + name + ":\n  writers: " << writer_count << "\n  readers: " << reader_count << "\n  items waiting: " << fifo.size() << "\n";
        }


      private:
        Mutex mutex;
        Cond more_data, more_space;
        std::queue<T*> fifo;
        size_t capacity;
        size_t writer_count, reader_count;
        std::string name;

        void register_writer ()   { Mutex::Lock lock (mutex); ++writer_count; }
        void unregister_writer () { 
          Mutex::Lock lock (mutex);
          assert (writer_count); 
          --writer_count; 
          if (!writer_count) {
            debug ("no writers left on queue \"" + name + "\"");
            more_data.broadcast(); 
          }
        }
        void register_reader ()   { Mutex::Lock lock (mutex); ++reader_count; }
        void unregister_reader () { 
          Mutex::Lock lock (mutex); 
          assert (reader_count);
          --reader_count; 
          if (!reader_count) {
            debug ("no readers left on queue \"" + name + "\"");
            more_space.broadcast(); 
          }
        }

        bool push (T* item) { 
          Mutex::Lock lock (mutex); 
          while (fifo.size() >= capacity && reader_count) more_space.wait();
          if (!reader_count) return (false);
          fifo.push (item);
          more_data.signal();
          return (true);
        }

        T* pop () {
          Mutex::Lock lock (mutex);
          while (fifo.empty() && writer_count) more_data.wait();
          if (fifo.empty() && !writer_count) return (NULL);
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

