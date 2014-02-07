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

#ifndef __mrtrix_thread_queue_h__
#define __mrtrix_thread_queue_h__

#include <stack>

#include "ptr.h"
#include "thread/condition.h"
#include "thread/exec.h"

#define MRTRIX_QUEUE_DEFAULT_CAPACITY 128
#define MRTRIX_QUEUE_DEFAULT_BATCH_SIZE 128

namespace MR
{
  namespace Thread
  {


    //* \cond skip
    namespace {

      /********************************************************************
       * convenience Functor classes for use in Thread::run_queue()
       ********************************************************************/

      template <class Functor> 
        class __Multi {
          public:
            __Multi (Functor& object, size_t number) : obj (object), num (number) { }
            template <class X> bool operator() (const X& x) { assert (0); return false; }
            template <class X> bool operator() (X& x) { assert (0); return false; }
            template <class X, class Y> bool operator() (const X& x, Y& y) { assert (0); return false; }
            Functor& obj;
            size_t num;
        };

      template <class Item> 
        class __Batch {
          public:
            __Batch (size_t number) : num (number) { }
            size_t num;
        };



      // to handle batched / unbatched seamlessly:
      template <class X> class __item { public: typedef X type; }; 
      template <class X> class __item < __Batch<X> > { public: typedef X type; };

      // to get multi/single job/functor seamlessly:
      template <class X>
        class __job
        {
          public:
            typedef X type;
            static X& functor (const X& job) { return const_cast<X&> (job); }
            static bool single_thread () { return true; }
        };

      template <class X>
        class __job < __Multi<X> >
        {
          public:
            typedef X type;
            static X& functor (const __Multi<X>& job) { return const_cast<X&> (job.obj); }
            static bool single_thread () { return false; }
        };

      // to launch multi/single job/functor seamlessly:
      template <class F, class T> 
        class __exec 
        { 
          public:
            __exec (const F& f, T& t, const char* name) : launcher (t, name) { }
          private:
            Exec launcher;
        };

      template <class F, class T> 
        class __exec <__Multi<F>, T>
        { 
          public:
            __exec (const __Multi<F>& f, T& t, const char* name) : 
              array (t, f.num),
              launcher (array, name) { }
          private:
            Array<T> array;
            Exec launcher;
        };

    }

    //! \endcond 




    /** \addtogroup Thread
     * @{ */






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
     *   - create an instance of Thread::Queue::Writer::Item, constructed from
     *   the corresponding Thread::Queue::Writer;
     *   - perform processing in a loop:
     *     - prepare the item using pointer semantics (i.e. *item or
     *     item->method());
     *     - use the write() method of this class to write to the queue;
     *     - break out of loop if write() returns \c false.
     *   - when the execute() method returns, the destructor of the
     *   Thread::Queue::Writer::Item class will notify the queue that its
     *   thread has finished writing to the queue.
     * - Within the execute() method of each thread with a
     * Thread::Queue::Reader:
     *   - create an instance of Thread::Queue::Reader::Item, constructed from
     *   the corresponding Thread::Queue::Reader;
     *   - perform processing in a loop:
     *     - use the read() method of this class to read the next item from the
     *     queue;
     *     - break out of the loop if read() returns \c false;
     *     - process the item using pointer semantics (i.e. *item or
     *     item->method()).
     *   - when the execute() method returns, the destructor of the
     *   Thread::Queue::Reader::Item class will notify the queue that its
     *   thread has finished reading from the queue.
     * - If all reader threads have returned, the queue will notify all writer
     * threads that processing should stop, by returning \c false from the next
     * write attempt.
     * - If all writer threads have returned and no items remain in the queue,
     * the queue will notify all reader threads that processing should stop, by
     * returning \c false from the next read attempt.
     *
     * The additional member classes are designed to be used in conjunction
     * with the MRtrix multi-threading interface. In this system, each thread
     * corresponds to an instance of a functor class, and its execute() method
     * is the function that will be run within the thread (see Thread::Exec for
     * details). For this reason:
     * - The Thread::Queue instance is designed to be created before any of the
     *   threads.
     * - The Thread::Queue::Writer and Thread::Queue::Reader classes are
     *   designed to be used as members of each functor, so that each functor
     *   must construct these classes from a reference to the queue within
     *   their own constructor. This ensures each thread registers their
     *   intention to read or write with the queue \e before their thread is
     *   launched.
     * - The Thread::Queue::Writer::Item and Thread::Queue::Reader::Item
     *   classes are designed to be instantiated within each functor's
     *   execute() method. They must be constructed from a reference to a
     *   Thread::Queue::Writer or Thread::Queue::Reader respectively, ensuring
     *   no reads or write can take place without having registered with the
     *   queue. Their destructors will also unregister from the queue, ensuring
     *   that each thread unregisters as soon as the execute() method returns,
     *   and hence \e before the thread exits.
     *
     * The Queue class performs all memory management for the items in the
     * queue. For this reason, the items are accessed via the Writer::Item &
     * Reader::Item classes. This allows items to be recycled once they have
     * been processed, reducing overheads associated with memory
     * allocation/deallocation. 
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
     * // The use a typedef is recommended to help with readability (and typing!):
     * typedef Thread::Queue<Item> MyQueue;
     *
     *
     * // this class will write to the queue:
     * class Sender {
     *   public:
     *     // construct the 'writer' member in the constructor:
     *     Sender (MyQueue& queue) : writer (queue) { }
     *
     *     void execute () {
     *       // use a local instance of Thread::Queue<Item>::Writer::Item to write to the queue:
     *       MyQueue::Writer::Item item (writer);
     *       while (need_more_items()) {
     *         ...
     *         // prepare item
     *         *item = something();
     *         item->set (something_else);
     *         ...
     *         if (!item.write()) break; // break if write() returns false
     *       }
     *     }
     *
     *   private:
     *     MyQueue::Writer writer;
     * };
     *
     *
     * // this class will read from the queue:
     * class Receiver {
     *   public:
     *     // construct the 'reader' member in the constructor:
     *     Receiver (MyQueue& queue) : reader (queue) { }
     *
     *     void execute () {
     *       // use a local instance of Thread::Queue<Item>::Reader::Item to read from the queue:
     *       MyQueue::Reader::Item item (reader);
     *       while ((item.read())) { // break when read() returns false
     *         ...
     *         // process item
     *         do_something (*item);
     *         if (item->status()) report_error();
     *         ...
     *         if (enough_items()) return;
     *       }
     *     }
     *
     *   private:
     *     MyQueue::Reader reader;
     * };
     *
     *
     * // this is where the queue and threads are created:
     * void my_function () {
     *   // create an instance of the queue:
     *   MyQueue queue;
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
     * \par Rationale for the Writer, Reader, and Item member classes
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
     *
     * While this could have been achieved simply with the appropriate member
     * functions (i.e. register(), unregister(), %read() & %write() methods in
     * the main Queue class), this places a huge burden on the developer to get
     * it right. Using these member functions reduces the chance of coding
     * errors, and in fact reduces the total amount of code that needs to be
     * written to use the Queue in a safe manner.
     *
     * The Item classes additionally simplify the memory management of the
     * items in the queue, by preventing direct access to the underlying
     * pointers, and ensuring the Queue itself is responsible for all
     * allocation and deallocation of items as needed.
     */
    template <class T> class Queue
    {
      public:
        //! Construct a Queue of items of type \c T
        /*! \param description a string identifying the queue for degugging purposes
         * \param buffer_size the maximum number of items that can be pushed onto the queue before
         * blocking. If a thread attempts to push more data onto the queue when the
         * queue already contains this number of items, the thread will block until
         * at least one item has been popped.  By default, the buffer size is
         * MRTRIX_QUEUE_DEFAULT_CAPACITY items.
         */
        Queue (const std::string& description = "unnamed", size_t buffer_size = MRTRIX_QUEUE_DEFAULT_CAPACITY) :
          more_data (mutex),
          more_space (mutex),
          buffer (new T* [buffer_size]),
          front (buffer),
          back (buffer),
          capacity (buffer_size),
          writer_count (0),
          reader_count (0),
          name (description) {
          assert (capacity > 0);
        }

        Queue (const T& item_type, const std::string& description = "unnamed", size_t buffer_size = MRTRIX_QUEUE_DEFAULT_CAPACITY) :
          more_data (mutex),
          more_space (mutex),
          buffer (new T* [buffer_size]),
          front (buffer),
          back (buffer),
          capacity (buffer_size),
          writer_count (0),
          reader_count (0),
          name (description) {
          assert (capacity > 0);
        }


        ~Queue () {
          delete [] buffer;
        }

        //! This class is used to register a writer with the queue
        /*! Items cannot be written directly onto a Thread::Queue queue. An
         * object of this class must first be instanciated to notify the queue
         * that a section of code will be writing to the queue. The actual
         * process of writing items to the queue is done via the Writer::Item
         * class.
         *
         * See Thread::Queue for more information and an example. */
        class Writer
        {
          public:
            //! Register a Writer object with the queue
            /*! The Writer object will register itself with the queue as a
             * writer. */
            Writer (Queue<T>& queue) : Q (queue) {
              Q.register_writer();
            }
            Writer (const Writer& W) : Q (W.Q) {
              Q.register_writer();
            }

            //! This class is used to write items to the queue
            /*! Items cannot be written directly onto a Thread::Queue queue. An
             * object of this class must be instantiated and used to write to the
             * queue.
             *
             * See Thread::Queue for more information and an example. */
            class Item
            {
              public:
                //! Construct a Writer::Item object
                /*! The Writer::Item object can only be instantiated from a
                 * Writer object, ensuring that the corresponding section of code
                 * has already registered as a writer with the queue. The
                 * destructor for this object will unregister from the queue.
                 *
                 * \note There should only be one Writer::Item object per Writer.
                 * */
                Item (const Writer& writer) : Q (writer.Q), p (Q.get_item()) { }
                //! Unregister the parent Writer from the queue
                ~Item () {
                  Q.unregister_writer();
                }
                //! Push the item onto the queue
                bool write () {
                  return (Q.push (p));
                }
                T& operator*() const throw ()   {
                  return (*p);
                }
                T* operator->() const throw ()  {
                  return (p);
                }
              private:
                Queue<T>& Q;
                T* p;
            };

          private:
            Queue<T>& Q;
        };


        //! This class is used to register a reader with the queue
        /*! Items cannot be read directly from a Thread::Queue queue. An
         * object of this class must be instanciated to notify the queue
         * that a section of code will be reading from the queue. The actual
         * process of reading items from the queue is done via the Reader::Item
         * class.
         *
         * See Thread::Queue for more information and an example. */
        class Reader
        {
          public:
            //! Register a Reader object with the queue.
            /*! The Reader object will register itself with the queue as a
             * reader. */
            Reader (Queue<T>& queue) : Q (queue) {
              Q.register_reader();
            }
            Reader (const Reader& reader) : Q (reader.Q) {
              Q.register_reader();
            }

            //! This class is used to read items from the queue
            /*! Items cannot be read directly from a Thread::Queue queue. An
             * object of this class must be instanciated and used to read from the
             * queue.
             *
             * See Thread::Queue for more information and an example. */
            class Item
            {
              public:
                //! Construct a Reader::Item object
                /*! The Reader::Item object can only be instantiated from a
                 * Reader object, ensuring that the corresponding section of code
                 * has already registered as a reader with the queue. The
                 * destructor for this object will unregister from the queue.
                 *
                 * \note There should only be one Reader::Item object per
                 * Reader. */
                Item (const Reader& reader) : Q (reader.Q), p (NULL) { }
                //! Unregister the parent Reader from the queue
                ~Item () {
                  Q.unregister_reader();
                }
                //! Get next item from the queue
                bool read () {
                  return (Q.pop (p));
                }
                T& operator*() const throw ()   {
                  return (*p);
                }
                T* operator->() const throw ()  {
                  return (p);
                }
                bool operator! () const throw () {
                  return !p;
                }
              private:
                Queue<T>& Q;
                T* p;
            };
          private:
            Queue<T>& Q;
        };

        //! Print out a status report for debugging purposes
        void status () {
          Mutex::Lock lock (mutex);
          std::cerr << "Thread::Queue \"" + name + "\": "
                    << writer_count << " writer" << (writer_count > 1 ? "s" : "") << ", "
                    << reader_count << " reader" << (reader_count > 1 ? "s" : "") << ", items waiting: " << size() << "\n";
        }


      private:
        Mutex mutex;
        Cond more_data, more_space;
        T** buffer;
        T** front;
        T** back;
        size_t capacity;
        size_t writer_count, reader_count;
        std::stack<T*,std::vector<T*> > item_stack;
        VecPtr<T> items;
        std::string name;

        Queue (const Queue& queue) {
          assert (0);
        }
        Queue& operator= (const Queue& queue) {
          assert (0);
          return (*this);
        }

        void register_writer ()   {
          Mutex::Lock lock (mutex);
          ++writer_count;
        }
        void unregister_writer () {
          Mutex::Lock lock (mutex);
          assert (writer_count);
          --writer_count;
          if (!writer_count) {
            DEBUG ("no writers left on queue \"" + name + "\"");
            more_data.broadcast();
          }
        }
        void register_reader ()   {
          Mutex::Lock lock (mutex);
          ++reader_count;
        }
        void unregister_reader () {
          Mutex::Lock lock (mutex);
          assert (reader_count);
          --reader_count;
          if (!reader_count) {
            DEBUG ("no readers left on queue \"" + name + "\"");
            more_space.broadcast();
          }
        }

        bool empty () const {
          return (front == back);
        }
        bool full () const {
          return (inc (back) == front);
        }
        size_t size () const {
          return ( (back < front ? back+capacity : back) - front);
        }

        T* get_item () {
          Mutex::Lock lock (mutex);
          T* item = new T;
          items.push_back (item);
          return (item);
        }

        bool push (T*& item) {
          Mutex::Lock lock (mutex);
          while (full() && reader_count) more_space.wait();
          if (!reader_count) return (false);
          *back = item;
          back = inc (back);
          more_data.signal();
          if (item_stack.empty()) {
            item = new T;
            items.push_back (item);
          }
          else {
            item = item_stack.top();
            item_stack.pop();
          }
          return (true);
        }

        bool pop (T*& item) {
          Mutex::Lock lock (mutex);
          if (item) 
            item_stack.push (item);
          item = NULL;
          while (empty() && writer_count) more_data.wait();
          if (empty() && !writer_count) return (false);
          item = *front;
          front = inc (front);
          more_space.signal();
          return (true);
        }

        T** inc (T** p) const {
          ++p;
          if (p >= buffer + capacity) p = buffer;
          return (p);
        }
    };






     //* \cond skip

    template <class T> class Queue< __Batch<T> >
    {
      private:
        typedef std::vector<T> BatchType;
        typedef Queue<BatchType> BatchQueue;

      public:
        Queue (const __Batch<T>& item_type, const std::string& description = "unnamed", size_t buffer_size = MRTRIX_QUEUE_DEFAULT_CAPACITY) :
          batch_queue (description, buffer_size),
          batch_size (item_type.num) { }


        class Writer
        {
          public:
            Writer (Queue< __Batch<T> >& queue) : batch_writer (queue.batch_queue), batch_size (queue.batch_size) { }

            class Item
            {
              public:
                Item (const Writer& writer) : 
                  batch_item (writer.batch_writer), batch_size (writer.batch_size), n (0) { 
                    batch_item->resize (batch_size);
                }
                ~Item () {
                  if (n) {
                    batch_item->resize (n);
                    batch_item.write();
                  }
                }
                bool write () {
                  if (++n >= batch_size) {
                    if (!batch_item.write()) 
                      return false;
                    n = 0;
                    batch_item->resize (batch_size);
                  }
                  return true;
                }
                T& operator*() const throw ()   {
                  return (*batch_item)[n];
                }
                T* operator->() const throw ()  {
                  return &((*batch_item)[n]);
                }
              private:
                typename BatchQueue::Writer::Item batch_item;
                const size_t batch_size;
                size_t n;
            };

          private:
            typename BatchQueue::Writer batch_writer;
            const size_t batch_size;
        };


        class Reader
        {
          public:
            Reader (Queue< __Batch<T> >& queue) : batch_reader (queue.batch_queue), batch_size (queue.batch_size) { }

            class Item
            {
              public:
                Item (const Reader& reader) : batch_item (reader.batch_reader), batch_size (reader.batch_size), n (0) { }
                bool read () {
                  if (!batch_item) 
                    return batch_item.read();

                  if (++n >= batch_item->size()) {
                    if (!batch_item.read()) 
                      return false;
                    n = 0;
                  }
                  return true;
                }
                T& operator*() const throw ()   {
                  return (*batch_item)[n];
                }
                T* operator->() const throw ()  {
                  return &((*batch_item)[n]);
                }
              private:
                typename BatchQueue::Reader::Item batch_item;
                const size_t batch_size;
                size_t n;
            };
          private:
            typename BatchQueue::Reader batch_reader;
            const size_t batch_size;
        };

        void status () { batch_queue.status(); }


      private:
        BatchQueue batch_queue;
        const size_t batch_size;
    };


    namespace {

       template <class Type, class Functor>
         class __Source
         {
           public:
             __Source (Queue<Type>& queue, Functor& functor) :
               writer (queue), func (__job<Functor>::functor (functor)) { }
             __Source (const __Source& S) :
               writer (S.writer), funcp (new Functor (S.func)), func (*funcp) { }

             void execute () {
               typename Queue<Type>::Writer::Item out (writer);
               do {
                 if (!func (*out)) 
                   return;
               } while (out.write());
             }

           private:
             typename Queue<Type>::Writer writer;
             Ptr<typename __job<Functor>::type> funcp;
             typename __job<Functor>::type& func;
         };


       template <class Type1, class Functor, class Type2>
         class __Pipe
         {
           public:
             typedef typename __job<Functor>::type Functype;

             __Pipe (Queue<Type1>& queue_in, Functor& functor, Queue<Type2>& queue_out) :
               reader (queue_in), writer (queue_out), func (__job<Functor>::functor (functor)) { }
             __Pipe (const __Pipe& P) :
               reader (P.reader), writer (P.writer), funcp (new Functor (P.func)), func (*funcp) { }

             void execute () {
               typename Queue<Type1>::Reader::Item in (reader);
               typename Queue<Type2>::Writer::Item out (writer);
               do {
                 if (!in.read()) return;
                 if (!func (*in, *out))
                   return;
               }
               while (out.write());
             }

           private:
             typename Queue<Type1>::Reader reader;
             typename Queue<Type2>::Writer writer;
             Ptr<typename __job<Functor>::type> funcp;
             typename __job<Functor>::type& func;
         };



       template <class Type, class Functor>
         class __Sink
         {
           public:
             typedef typename __job<Functor>::type Functype;

             __Sink (Queue<Type>& queue, Functor& functor) :
               reader (queue), func (__job<Functor>::functor (functor)) { }
             __Sink (const __Sink& S) :
               reader (S.reader), funcp (new Functor (S.func)), func (*funcp) { }

             void execute () {
               typename Queue<Type>::Reader::Item in (reader);
               while (in.read()) {
                 if (!func (*in))
                   return;
               }
             }

           private:
             typename Queue<Type>::Reader reader;
             Ptr<typename __job<Functor>::type> funcp;
             typename __job<Functor>::type& func;
         };


    }


    //* \endcond






    //! convenience functions to set up and run multi-threaded queues.
    /*! This set of convenience functions simplify the process of setting up a
     * multi-threaded processing chain that should meet most users' needs.
     * The differences between the three functions are as follows:
     * - run_queue_threaded_source(): Multiple 'source' threads feed data
     *     onto the queue, which are received by a single 'sink' thread.
     * - run_queue_threaded_sink(): A single 'source' thread feeds data
     *     onto the queue, which are then received and processed by multiple
     *     'sink' threads.
     * - run_queue_threaded_pipe(); A 'source' thread places data onto a
     *     queue. These data are processed by multiple 'pipe' threads,
     *     which place the processed data onto a second queue. A single
     *     'sink' thread then reads the data from this second queue.
     * 
     * The template types consist of the functor classes, and
     * the type(s) of the item(s) that will be passed through the queue(s).
     * Note that in the case of run_queue_threaded_pipe(), there are two
     * queues involved, and these queues may have different types.
     *
     * The actual arguments correspond to an instance of both the source &
     * sink functors (as well as the pipe functor if run_queue_threaded_pipe()
     * is used), and an item instance for each queue (provided purely to specify
     * the type of object to pass through the queue(s)).
     *
     * Where more than one thread is to be used for a functor, additional
     * instances of the functor will be created using the relevant class's
     * copy constructor. Therefore, care should be taken to ensure that the
     * copy constructor behaves appropriately.
     *
     * For the functor that is being multi-threaded, the number of threads
     * instantiated will depend on the "NumberOfThreads" entry in the MRtrix
     * confugration file, or can be set at the command-line using the
     * -nthreads option.
     *
     * Note that if the number of threads is set to 1, then no additional
     * threads will be spawned; the function will use local data storage and
     * explicitly execute the relevant functors within the calling thread.
     *
     *
     * \par Source: the input functor 
     * The Source class must at least provide the method:
     * \code 
     * bool operator() (Type& item);
     * \endcode
     * This function prepares the \a item passed to it, and should return \c
     * true if further items need to be prepared, or \c false to signal that
     * no further items are to be sent through the queue (at which point the
     * corresponding thread(s) will exit).
     *
     * \par Sink: the output functor 
     * The Sink class must at least provide the method:
     * \code 
     * bool operator() (const Type& item);
     * \endcode
     * This function processes the \a item passed to it, and should return \c
     * true when ready to process further items, or \c false to signal the end
     * of processing (at which point the corresponding thread(s) will exit).
     *
     * \par Pipe: the processing functor (for run_queue_threaded_pipe() only)
     * The Pipe class must at least provide the method:
     * \code 
     * bool operator() (const Type1& item_in, Type2& item_out);
     * \endcode
     * This function processes the \a item_in passed to it, and prepares
     * \a item_out for the next stage of the pipeline. It should return \c
     * true when ready to process further items, or \c false to signal the end
     * of processing (at which point the corresponding thread(s) will exit).
     *
     *
     * \par Example usage
     * \code
     * // The item type to be passed through the queue:
     * class Item {
     *   public:
     *     size_t n;
     * };
     *
     *
     * // The Source class prepares each item to be sent through the queue:
     * class Source {
     *   public:
     *     Source () : count (0) { }
     *
     *     // The kernel responsible for preparing each item to be sent.
     *     // This simply generates 1000000 items with incrementing count.
     *     bool operator() (Item& item) {
     *       item.n = count++;
     *       return count < 1000000;
     *     }
     *
     *     size_t count;
     * };
     *
     * // The Pipe class processes data from one queue and places the result
     * //   onto another:
     * class Pipe {
     *   public:
     *     Pipe () { }
     *
     *     // The kernel responsible for processing the data.
     *     // This simply doubles each value received.
     *     bool operator() (const Item& item_in, Item& item_out) {
     *       item_out.n = 2 * item_in.n;
     *       return true;
     *     }
     * };
     *
     * // The Sink class processes each item received through the queue:
     * class Sink {
     *   public:
     *     Sink () : count (0) { }
     *
     *     // The kernel responsible for processing each item received.
     *     // This example simply adds up the counts of the items received.
     *     bool operator() (const Item& item) {
     *       count += item.n;
     *       return true;
     *     }
     *
     *     size_t count;
     * };
     *
     * int main (...) 
     * {
     *   Source source;
     *   Pipe pipe;
     *   Sink sink;
     *
     *   // Run many threads for the source and one thread for the sink:
     *   run_queue_threaded_source (source, Item(), sink);
     *
     *   // Run one thread for the source and many threads for the sink:
     *   run_queue_threaded_sink   (source, Item(), sink);
     *
     *   // Run one source thread, one sink thread and many pipe threads:
     *   run_queue_threaded_pipe   (source, Item(), pipe, Item(), sink);
     *
     * }
     * \endcode
     */





    //! used to request multiple threads of the corresponding functor
    template <class Functor>
      inline __Multi<Functor> multi (Functor& object, size_t number = number_of_threads()) 
      {
        return __Multi<Functor> (object, number);
      }

 
    //! used to request batched processing of items
    template <class Item>
      inline __Batch<Item> batch (const Item& object, size_t number = MRTRIX_QUEUE_DEFAULT_BATCH_SIZE) 
      {
        return __Batch<Item> (number);
      }




    //! functors not const as advertised!
    template <class Source, class Type, class Sink>
      inline void run_queue (
          const Source& source, 
          const Type& item_type, 
          const Sink& sink, 
          size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY)
      {
        if (__job<Source>::single_thread() && __job<Sink>::single_thread()) {
          typename __item<Type>::type item;
          while (const_cast<Source&> (source) (item)) 
            if (!const_cast<Sink&> (sink) (item))
              return;
          return;
        }

        typedef typename __job<Source>::type SourceType;
        typedef typename __job<Sink>::type SinkType;

         Queue<Type> queue (item_type, "source->sink", capacity);
         __Source<Type,SourceType> source_functor (queue, __job<Source>::functor (source));
         __Sink<Type,SinkType>     sink_functor   (queue, __job<Sink>::functor (sink));

         __exec<Source, __Source<Type,SourceType> > source_threads (source, source_functor, "source");
         __exec<Sink, __Sink<Type,SinkType> >       sink_threads   (sink, sink_functor, "sink");
      }




    //! functors not const as advertised!
    template <class Source, class Type1, class Pipe, class Type2, class Sink>
      inline void run_queue (
          const Source& source,
          const Type1& item_type1, 
          const Pipe& pipe, 
          const Type2& item_type2, 
          const Sink& sink, 
          size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY)
      {
        if (__job<Source>::single_thread() && __job<Pipe>::single_thread() && __job<Sink>::single_thread()) {
          typename __item<Type1>::type item1;
          typename __item<Type2>::type item2;
          while (const_cast <Source&> (source) (item1)) {
            if (!const_cast<Pipe&> (pipe) (item1, item2))
              return;
            if (!const_cast<Sink&> (sink) (item2))
              return;
          }
          return;
        }


        typedef typename __job<Source>::type SourceType;
        typedef typename __job<Pipe>::type PipeType;
        typedef typename __job<Sink>::type SinkType;

         Queue<Type1> queue1 (item_type1, "source->pipe", capacity);
         Queue<Type2> queue2 (item_type2, "pipe->sink", capacity);

         __Source<Type1,SourceType>   source_functor (queue1, __job<Source>::functor (source));
         __Pipe<Type1,PipeType,Type2> pipe_functor   (queue1, __job<Pipe>::functor (pipe), queue2);
         __Sink<Type2,SinkType>       sink_functor   (queue2, __job<Sink>::functor (sink));

         __exec<Source, __Source<Type1,SourceType> >   source_threads (source, source_functor, "source");
         __exec<Pipe, __Pipe<Type1,PipeType,Type2> >   pipe_threads (pipe, pipe_functor, "pipe");
         __exec<Sink, __Sink<Type2,SinkType> >         sink_threads (sink, sink_functor, "sink");
      }






    /** @} */
  }
}

#endif


