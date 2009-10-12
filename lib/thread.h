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

    bool initialised ();
    //! Initialise the thread system
    /*! This function must be called before using any of the functionality in
     * the multi-threading system. */
    void init ();

    /*! the number of cores to use for multi-threading, as specified in the
     * variable NumberOfThreads in the MRtrix configuration file */
    size_t number ();
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
     * Thread::Mutex mutex;
     * Thread::Cond cond (mutex);
     * ...
     * void process_data () {
     *   while (true) {
     *     { // Mutex must be locked prior to waiting on condition
     *       Thread::Mutex::Lock lock (mutex);
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
     *       Thread::Mutex::Lock lock (mutex);
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




    //! Create an array of duplicate functors to execute in parallel
    /*! Use this class to hold an array of duplicates of the supplied functor,
     * so that they can be executed in parallel using the Thread::Exec class.
     * Note that the original functor will be used, and as many copies will be
     * created as needed to make up a total of \a num_threads. By default, \a
     * num_threads is given by Thread::number(). 
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
     *   // Duplicate as needed up to a maximum of Thread::number() threads.
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
        Array (Functor& functor, size_t num_threads = number()) :
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
          ID (1), name (description) { start (ID[0], functor); }

        //! Start an array of new threads each runnning the execute() method of its \a functor
        /*! A human-readable identifier can be supplied via the \a description
         * parameter. This is helping for debugging and error reporting. */
        template <class Functor> Exec (Array<Functor>& functor, const std::string& description = "unnamed") : 
          ID (functor.functors.size()+1), name (description) {
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
        }

      private:
        std::vector<pthread_t> ID;
        std::string name;

        template <class Functor> void start (pthread_t& id, Functor& functor) { 
          if (pthread_create (&id, default_attributes(), static_exec<Functor>, static_cast<void*> (&functor))) 
            throw Exception (std::string("error launching thread \"" + name + "\": ") + strerror (errno));
          debug ("launched thread \"" + name + "\" [ID " + str(id) + "]..."); 
        }
        template <class Functor> static void* static_exec (void* data) { static_cast<Functor*>(data)->execute (); return (NULL); }
    };


    


    //! A first-in first-out thread-safe item queue
    /*! This class implements a thread-safe means of pushing data items into a
     * queue, so that they can each be processed in one or more separate
     * threads. It has somewhat unusual usage, which consists of the following
     * steps:
     * - Create an instance of a Thread::Queue
     * - Create one or more instances of the corresponding
     * Thread::Queue::Writer class, each constructed with a reference to the
     * queue. Each of these instances will automatically notify the queue that
     * its corresponding thread will be writing to the queue.
     * - Create one or more instances of the corresponding
     * Thread::Queue::Reader class, each constructed with a reference to the
     * queue. Each of these instances will automatically notify the queue that
     * its corresponding thread will be reading from the queue.
     * - Launch all threads, one per instance of Thread::Queue::Writer or
     *   Thread::Queue::Reader. Note that one of these threads can be the
     *   current thread - simply invoke the respective functor's execute()
     *   method directly once all other threads have been launched.
     * - Within the execute() method of each thread with a
     * Thread::Queue::Writer:
     *   - create an instance of Thread::Queue::Write, constructed from
     *   the corresponding Thread::Queue::Writer;
     *   - use the operator() method of this class to write to the queue;
     *   - when the execute() method returns, the destructor of the
     *   Thread::Queue::Write class will notify the queue that its thread
     *   has finished writing to the queue.
     * - Within the execute() method of each thread with a
     * Thread::Queue::Reader:
     *   - create an instance of Thread::Queue::Read, constructed from
     *   the corresponding Thread::Queue::Reader;
     *   - use the operator() method of this class to read from the queue;
     *   - when the execute() method returns, the destructor of the
     *   Thread::Queue::Read class will notify the queue that its thread
     *   has finished reading from the queue.
     * - If all reader threads have returned, the queue will notify all writer
     * threads that processing should stop, by returning \c false from the nest
     * write attempt.
     * - If all writer threads have returned and no items remain in the queue,
     * the queue will notify all reader threads that processing should stop, by
     * returning \c NULL from the next read attempt.
     *
     * The additional member classes are designed to be used in conjunction
     * with the MRtrix multi-threading interface. In this system, each thread
     * corresponds to an instance of a functor class, and its execute() method
     * is the function that will be run within the thread (see Thread::Exec for
     * details). For this reason:
     * - The Thread::Queue instance is designed to be created before any of
     * the threads.
     * - The Thread::Queue::Writer and Thread::Queue::Reader classes are
     * designed to be used as members of each functor, so that each functor
     * must construct these classes from a reference to the queue within their
     * own constructor. This ensures each thread registers their intention to
     * read or write with the queue \e before their thread is launched.
     * - The Thread::Queue::Write and Thread::Queue::Read classes are
     * designed to be instanciated within each functor's execute() method.
     * They must be constructed from a reference to a Thread::Queue::Writer
     * or Thread::Queue::Reader respectively, ensuring no reads or write can
     * take place without having registered with the queue. Their destructors
     * will also unregister from the queue, ensuring that each thread
     * unregisters as soon as the execute() method returns, and hence \e before
     * the thread exits.
     *
     * \note It is important that all instances of Thread::Queue::Writer and
     * Thread::Queue::Reader are created \e before any of the threads are
     * launched, to avoid any race conditions at startup.
     * 
     * The use of Thread::Queue is best illustrated with an example:
     * \code
     * // the type of objects that will be sent through the queue:
     * class Item {
     *   public:
     *     ...
     *     // data members
     *     ...
     * };
     *
     *
     * // this class will write to the queue:
     * class Sender {
     *   public:
     *     // construct the 'writer' member in the constructor:
     *     Sender (Thread::Queue<Item>& queue) : writer (queue) { } 
     *
     *     void execute () {
     *       // use a local instance of Thread::Queue<Item>::Write to write to the queue:
     *       Thread::Queue<Item>::Write write (writer);
     *       while (need_more_items()) {
     *         Item* item = new Item;
     *         ...
     *         // prepare item
     *         ...
     *         if (!write (item)) break; // break if write() returns false
     *       }
     *     }
     *
     *   private:
     *     Thread::Queue<Item>::Writer writer;
     * };
     *
     * 
     * // this class will read from the queue:
     * class Receiver {
     *   public:
     *     // construct the 'reader' member in the constructor:
     *     Receiver (Thread::Queue<Item>& queue) : reader (queue) { } 
     *
     *     void execute () {
     *       Item* item;
     *       // use a local instance of Thread::Queue<Item>::Read to read from the queue:
     *       Thread::Queue<Item>::Read read (reader);
     *       while ((item = read())) { // break when read() returns NULL
     *         ...
     *         // process item
     *         ...
     *         delete item;
     *         if (enough_items()) return; 
     *       }
     *     }
     *
     *   private:
     *     Thread::Queue<Item>::Reader reader;
     * };
     *
     * 
     * // this is where the queue and threads are created:
     * void my_function () {
     *   // create an instance of the queue:
     *   Thread::Queue<Item> queue;
     *  
     *   // create all functors from a reference to the queue:
     *   Sender sender (queue);
     *   Receiver receiver (queue);
     *   
     *   // once all functors are created, launch their corresponding threads:
     *   Thread::Exec sender_thread (sender);
     *   Thread::Exec receiver_thread (receiver);
     * }
     * \endcode
     *
     * \par Rationale for the Reader/Writer and Read/Reader member classes
     *
     * The motivation for the use of additional member classes to perform the
     * actual process of writing and reading to and from the queue is related
     * to the need to keep track of the number of processes currently using the
     * queue. This is essential to ensure that threads are notified when the
     * queue is closed. This happens either when all readers have finished
     * reading; or when all writers have finished writing and no items are left
     * in the queue. This is complicated by the need to ensure that the various
     * operations are called in the right order to avoid deadlocks.
     *
     * There are essentially 4 operations that need to take place:
     * - registering an intention to read/write from/to the queue
     * - launching the corresponding thread
     * - unregistering from the queue
     * - terminating the thread
     *
     * For proper multi-threaded operations, these operations must take place
     * in the order above. Moreover, each operation must be completed for
     * all users of the queue before any of them can perform the next
     * operation. The use of additional member classes ensures that threads
     * have to register their intention to read or write from the queue, and
     * that they unregister from the queue once their processing is done. 
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
          buffer (new T* [buffer_size]), 
          front (buffer),
          back (buffer),
          capacity (buffer_size),
          writer_count (0),
          reader_count (0),
          name (description) { assert (capacity > 0); }
        ~Queue () { while (!empty()) { delete *front; front = inc(front); } delete [] buffer; }

        //! This class is used to register a writer with the queue
        /*! Items cannot be written directly onto a Thread::Queue queue. An
         * object of this class must first be instanciated to notify the queue
         * that a section of code will be writing to the queue. The actual
         * process of writing items to the queue is done via the Write class.
         * 
         * See Thread::Queue for more information and an example. */
        class Writer {
          public: 
            //! Register a Writer object with the queue
            /*! The Writer object will register itself with the queue as a
             * writer. */
            Writer (Queue<T>& queue) : Q (queue) { Q.register_writer(); }
            Writer (const Writer& W) : Q (W.Q) { Q.register_writer(); }
            friend class Queue<T>::Write;
          private:
            Queue<T>& Q;
        };

        //! This class is used to write items to the queue
        /*! Items cannot be written directly onto a Thread::Queue queue. An
         * object of this class must be instanciated and used to write to the
         * queue. 
         * 
         * See Thread::Queue for more information and an example. */
        class Write {
          public: 
            //! Construct a Write object 
            /*! The Write object can only be instantiated from a Writer object,
             * ensuring that the corresponding section of code has already
             * registered as a writer with the queue. The destructor for this
             * object will unregister from the queue. 
             *
             * \note There should only be one Write object per Writer. */
            Write (const Writer& writer) : Q (writer.Q) { }
            //! Unregister the Writer from the queue
            ~Write () { Q.unregister_writer(); }
            //! Push \a item onto the queue
            bool operator() (T* item) { return (Q.push (item)); }
          private:
            Queue<T>& Q;
        };

        //! This class is used to register a reader with the queue
        /*! Items cannot be read directly from a Thread::Queue queue. An
         * object of this class must be instanciated to notify the queue
         * that a section of code will be reading from the queue. The actual
         * process of reading items from the queue is done via the Read class.
         * 
         * See Thread::Queue for more information and an example. */
        class Reader {
          public: 
            //! Register a Reader object with the queue.
            /*! The Reader object will register itself with the queue as a
             * reader. */
            Reader (Queue<T>& queue) : Q (queue) { Q.register_reader(); }
            Reader (const Reader& reader) : Q (reader.Q) { Q.register_reader(); }
          private:
            Queue<T>& Q;
            friend class Queue<T>::Read;
        };

        //! This class is used to read items from the queue
        /*! Items cannot be read directly from a Thread::Queue queue. An
         * object of this class must be instanciated and used to read from the
         * queue.
         * 
         * See Thread::Queue for more information and an example. */
        class Read {
          public: 
            //! Construct a Read object 
            /*! The Read object can only be instantiated from a Writer object,
             * ensuring that the corresponding section of code has already
             * registered as a reader with the queue. The destructor for this
             * object will unregister from the queue. 
             *
             * \note There should only be one Read object per Reader. */
            Read (const Reader& reader) : Q (reader.Q) { }
            //! Unregister a reader from the queue
            ~Read () { Q.unregister_reader(); }
            //! Pop an item from the queue.
            T* operator() () { return (Q.pop()); }
          private:
            Queue<T>& Q;
        };

        //! Print out a status report for debugging purposes
        void status () { 
          Mutex::Lock lock (mutex);
          std::cerr << "Thread::Queue \"" + name + "\": " 
            << writer_count << " writer" << (writer_count > 1 ? "s" : "" ) << ", " 
            << reader_count << " reader" << (reader_count > 1 ? "s" : "" ) << ", items waiting: " << size() << "\n";
        }


      private:
        Mutex mutex;
        Cond more_data, more_space;
        T** buffer;
        T** front;
        T** back;
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

        bool empty () const { return (front == back); }
        bool full () const { return (inc(back) == front); }
        size_t size () const { return ((back < front ? back+capacity : back) - front); }

        bool push (T* item) { 
          Mutex::Lock lock (mutex); 
          while (full() && reader_count) more_space.wait();
          if (!reader_count) return (false);
          *back = item;
          back = inc (back);
          more_data.signal();
          return (true);
        }

        T* pop () {
          Mutex::Lock lock (mutex);
          while (empty() && writer_count) more_data.wait();
          if (empty() && !writer_count) return (NULL);
          T* item = *front;
          front = inc (front);
          more_space.signal();
          return (item);
        }

        T** inc (T** p) const { ++p; if (p >= buffer + capacity) p = buffer; return (p); }

        friend class Push;
        friend class Pop;
    };


    /** @} */
  }
}

#endif

