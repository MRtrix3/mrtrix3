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

#include <stack>

#include "ptr.h"
#include "thread/condition.h"
#include "thread/exec.h"

namespace MR
{
  namespace Thread
  {

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
            debug ("no writers left on queue \"" + name + "\"");
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
            debug ("no readers left on queue \"" + name + "\"");
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


     //! \cond skip

     template <class Type, class Functor>
       class __Source
       {
         public:
           __Source (Queue<Type>& queue, Functor& functor) :
             writer (queue), func (functor) { }

           __Source (const __Source& S) :
             writer (S.writer), funcp (new Functor (S.func)), func (*funcp) { }

           void execute () {
             typename Queue<Type>::Writer::Item out (writer);
             do {
               if (!func (*out))
                 return;
             }
             while (out.write());
           }

         private:
           typename Queue<Type>::Writer writer;
           Ptr<Functor> funcp;
           Functor& func;
       };


     template <class Type1, class Functor, class Type2>
       class __Pipe
       {
         public:
           __Pipe (Queue<Type1>& queue_in, Functor& functor, Queue<Type2>& queue_out) :
             reader (queue_in), writer (queue_out), func (functor) { }

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
           Ptr<Functor> funcp;
           Functor& func;
       };



     template <class Type, class Functor>
       class __Sink
       {
         public:
           __Sink (Queue<Type>& queue, Functor& functor) :
             reader (queue), func (functor) { }
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
           Ptr<Functor> funcp;
           Functor& func;
       };




     namespace Batch {

       template <class Type, class Functor>
         class __Source
         {
           public:
             __Source (Queue<std::vector<Type> >& queue, Functor& functor, size_t batch_size = 128) :
               writer (queue), func (functor), N (batch_size) { }

             __Source (const __Source& S) :
               writer (S.writer), funcp (new Functor (S.func)), func (*funcp), N (S.N) { }

             void execute () {
               typename Queue<std::vector<Type> >::Writer::Item out (writer);
               size_t n;
               do {
                 out->resize (N);
                 for (n = 0; n < N; ++n) {
                   if (!func ((*out)[n])) {
                     out->resize (n);
                     break;
                   }
                 }
                 if (!out.write()) 
                   return;
               } while (n == N);
             }

           private:
             typename Queue<std::vector<Type> >::Writer writer;
             Ptr<Functor> funcp;
             Functor& func;
             size_t N;
         };


       template <class Type1, class Functor, class Type2>
         class __Pipe
         {
           public:
             __Pipe (Queue<std::vector<Type1> >& queue_in,
                 Functor& functor,
                 Queue<std::vector<Type2> >& queue_out, size_t batch_size) :
               reader (queue_in), writer (queue_out), func (functor), N (batch_size) { }

             __Pipe (const __Pipe& P) :
               reader (P.reader), writer (P.writer), funcp (new Functor (P.func)), func (*funcp), N (P.N) { }

             void execute () {
               typename Queue<std::vector<Type1> >::Reader::Item in (reader);
               typename Queue<std::vector<Type2> >::Writer::Item out (writer);
               out->resize (N);
               size_t n1 = 0, n2 = 0;
               if (!in.read()) goto flush;
               if ((*in).empty()) goto flush;
               do {
                 if (!func ((*in)[n1], (*out)[n2])) goto flush;
                 ++n1;
                 ++n2;
                 if (n1 >= in->size()) {
                   if (!in.read()) goto flush;
                   if ((*in).empty()) goto flush;
                   n1 = 0;
                 }
                 if (n2 >= N) {
                   out.write();
                   out->resize (N);
                   n2 = 0;
                 }
               } while (true);

flush:
               if (n2) {
                 out->resize (n2);
                 out.write();
               }
             }

           private:
             typename Queue<std::vector<Type1> >::Reader reader;
             typename Queue<std::vector<Type2> >::Writer writer;
             Ptr<Functor> funcp;
             Functor& func;
             size_t N;
         };



       template <class Type, class Functor>
         class __Sink
         {
           public:
             __Sink (Queue<std::vector<Type> >& queue, Functor& functor) :
               reader (queue), func (functor) { }
             __Sink (const __Sink& S) :
               reader (S.reader), funcp (new Functor (S.func)), func (*funcp) { }

             void execute () {
               typename Queue<std::vector<Type> >::Reader::Item in (reader);
               while (in.read()) {
                 for (size_t n = 0; n < in->size(); ++n) 
                   if (!func ((*in)[n])) 
                     return;
               }
             }

           private:
             typename Queue<std::vector<Type> >::Reader reader;
             Ptr<Functor> funcp;
             Functor& func;
         };


    }


    //! \endcond 


     //! convenience function to set up and run a multi-threaded queue.
     /*! This is a convenience function to simplify the process of setting up a
      * multi-threaded processing chain that should meet most user's needs.
      * This function sets up a simple chain consisting of one or more input
      * (Source) functors feeding into a queue feeding into one or more output
      * (Sink) functors.
      * 
      * The template types consist of the input & output functor classes, and
      * the type of the item that will be passed through the queue from input
      * to output. 
      *
      * The actual arguments correspond to an instance of both the input &
      * output functors, the number of threads to use for each, and an item
      * instance (provided purely to specify the type of object to pass
      * through the queue).
      *
      * \note if the number of threads is set to zero, all available cores will
      * be used, as given by the Thread::available_cores() function.
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
      * If more than one thread are to be launched with this functor,
      * additional instances of the input functor will be created using the
      * Source class's copy constructor. In this case, care should be taken to
      * ensure the copy constructor behaves appropriately.
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
      * If more than one thread are to be launched with this functor,
      * additional instances of the output functor will be created using the
      * Sink class's copy constructor. In this case, care should be taken to
      * ensure the copy constructor behaves appropriately.
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
      *};
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
      *   Sink sink;
      *
      *   // Run one thread for the source and one thread for the sink:
      *   run_queue (source, 1, Item(), sink, 1);
      * }
      * \endcode
      * 
      * \sa run_queue<Source,Type1,Pipe,Type2,Sink>
      * \sa run_batched_queue<Source,Type,Sink>
      * \sa run_batched_queue<Source,Type1,Pipe,Type2,Sink>
      */
    template <class Source, class Type, class Sink>
      inline void run_queue (
          Source& source, size_t nthreads_source,
          const Type& item_type,
          Sink& sink, size_t nthreads_sink)
      {

        Queue<Type> queue;
        __Source<Type,Source> q_source (queue, source);
        __Sink<Type,Sink> q_sink (queue, sink);

        Array<__Source<Type,Source> > source_list (q_source, nthreads_source ? nthreads_source : available_cores());
        Array<__Sink<Type,Sink> > sink_list (q_sink, nthreads_sink ? nthreads_sink : available_cores());

        Exec source_threads (source_list, "source");
        Exec sink_threads (sink_list, "sink");
      }


     //! convenience function to set up and run a multi-threaded queue.
     /*! This is a convenience function to simplify the process of setting up a
      * multi-threaded processing chain that should meet most user's needs. 
      * This function sets up a three-step chain consisting of one or more input
      * (Source) functors feeding into a queue feeding into one or more
      * processing (Pipe) functors, themselves feeding into a second queue
      * feeding into one or more output (Sink) functors.
      * 
      * The template types consist of the input, processing & output functor
      * classes, and the types of the item that will be passed through each
      * queue from input to processor, and from processor to output. 
      *
      * The actual arguments correspond to an instance of the input, processing &
      * output functors, the number of threads to use for each, and item
      * instances for both queues (provided purely to specify the type of
      * object to pass through the queues).
      *
      * \note if the number of threads is set to zero, all available cores will
      * be used, as given by the Thread::available_cores() function.
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
      * If more than one thread are to be launched with this functor,
      * additional instances of the input functor will be created using the
      * Source class's copy constructor. In this case, care should be taken to
      * ensure the copy constructor behaves appropriately.
      *
      * \par Pipe: the processing functor 
      * The Pipe class must at least provide the method:
      * \code 
      * bool operator() (const Type1& item_in, Type2& item_out);
      * \endcode
      * This function processes the \a item_in passed to it, and prepares
      * \a item_out for the next stage of the pipeline. It should return \c
      * true when ready to process further items, or \c false to signal the end
      * of processing (at which point the corresponding thread(s) will exit).
      *
      * If more than one thread are to be launched with this functor,
      * additional instances of the processing functor will be created using the
      * Pipe class's copy constructor. In this case, care should be taken to
      * ensure the copy constructor behaves appropriately.
      *
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
      * If more than one thread are to be launched with this functor,
      * additional instances of the ouptut functor will be created using the
      * Sink class's copy constructor. In this case, care should be taken to
      * ensure the copy constructor behaves appropriately.
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
      *};
      *
      * // The Pipe class processes each input item received from the first
      * // queue and prepares an output item to be sent through the second queue:
      * class Pipe {
      *   public: 
      *     // The kernel responsible for processing each input item and
      *     // preparing each corresponding output item:
      *     // This example simply takes the remainder after division by 128.
      *     bool operator() (const Item& in, Item& out) {
      *       out.n = in.n % 128;
      *       return true;
      *     }
      * };
      * 
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
      *   // Run one thread for the source, all available cores for the
      *   // pipe, and one thread for the sink.
      *   // In this example, both queues will be passing through items of the
      *   // same type. 
      *   run_queue (source, 1, Item(), pipe, 0, Item(), sink, 1);
      * }
      * \endcode
      * 
      * \sa run_queue<Source,Type,Sink>
      * \sa run_batched_queue<Source,Type,Sink>
      * \sa run_batched_queue<Source,Type1,Pipe,Type2,Sink>
      */
    template <class Source, class Type1, class Pipe, class Type2, class Sink>
      inline void run_queue (
          Source& source, size_t nthreads_source, 
          const Type1& item_type1, 
          Pipe& pipe, size_t nthreads_pipe,
          const Type2& item_type2, 
          Sink& sink, size_t nthreads_sink)
      {
        Queue<Type1> queue1 ("queue1");
        Queue<Type2> queue2 ("queue2");

        __Source<Type1,Source> q_source (queue1, source);
        __Pipe<Type1,Pipe,Type2> q_pipe (queue1, pipe, queue2);
        __Sink<Type2,Sink> q_sink (queue2, sink);

        Array<__Source<Type1,Source> > source_list (q_source, nthreads_source ? nthreads_source : available_cores());
        Array<__Pipe<Type1,Pipe,Type2> > pipe_list (q_pipe, nthreads_pipe ? nthreads_pipe : available_cores());
        Array<__Sink<Type2,Sink> > sink_list (q_sink, nthreads_sink ? nthreads_sink : available_cores());

        Exec source_threads (source_list, "source");
        Exec pipe_threads (pipe_list, "pipe");
        Exec sink_threads (sink_list, "sink");
      }






     //! convenience function to set up and run a multi-threaded queue.
     /*! This is a convenience function to simplify the process of setting up a
      * multi-threaded processing chain that should meet most user's needs.
      * This function sets up a simple chain consisting of one or more input
      * (Source) functors feeding into a queue feeding into one or more output
      * (Sink) functors. 
      *
      * It differs from Thread::run_queue<Source,Type,Sink> in that batches of
      * items will be sent through the queue, rather than individual items.
      * This is beneficial for applications where the work required to process
      * each item is small, in which case the overhead of locking and unlocking
      * the queue for multi-threading becomes the bottleneck. By batching sets
      * of items together, the overhead of managing the queue can be reduced to
      * negligible levels. 
      *
      * Usage is almost identical to Thread::run_queue<Source,Type,Sink>, apart
      * from the call itself:
      * \code
      * int main (...) 
      * {
      *   Source source;
      *   Sink sink;
      *
      *   // Run one thread for the source and one thread for the sink,
      *   // with items sent in batches of 1024:
      *   run_queue (source, 1, Item(), 1024, sink, 1);
      * }
      * \endcode
      *
      * \sa run_queue<Source,Type,Sink>
      * \sa run_queue<Source,Type1,Pipe,Type2,Sink>
      * \sa run_batched_queue<Source,Type1,Pipe,Type2,Sink>
      */
    template <class Source, class Type, class Sink>
      inline void run_batched_queue (
          Source& source, size_t nthreads_source, 
          const Type& item_type, size_t batch_size,
          Sink& sink, size_t nthreads_sink)
      {

        Queue<std::vector<Type> > queue ("queue");
        Batch::__Source<Type,Source> q_source (queue, source, batch_size);
        Batch::__Sink<Type,Sink> q_sink (queue, sink);

        Array<Batch::__Source<Type,Source> > source_list (q_source, nthreads_source ? nthreads_source : available_cores());
        Array<Batch::__Sink<Type,Sink> > sink_list (q_sink, nthreads_sink ? nthreads_sink : available_cores());

        Exec source_threads (source_list, "source");
        Exec sink_threads (sink_list, "sink");
      }



     //! convenience function to set up and run a multi-threaded queue.
     /*! This is a convenience function to simplify the process of setting up a
      * multi-threaded processing chain that should meet most user's needs. 
      * This function sets up a three-step chain consisting of one or more input
      * (Source) functors feeding into a queue feeding into one or more
      * processing (Pipe) functors, themselves feeding into a second queue
      * feeding into one or more output (Sink) functors.
      *
      * It differs from Thread::run_queue<Source,Type1,Pipe,Type2,Sink> in that
      * batches of items will be sent through each queue, rather than
      * individual items. This is beneficial for applications where the work
      * required to process each item is small, in which case the overhead of
      * locking and unlocking the queues for multi-threading becomes the
      * bottleneck. By batching sets of items together, the overhead of
      * managing the queues can be reduced to negligible levels. 
      *
      * Usage is almost identical to
      * Thread::run_queue<Source,Type1,Pipe,Type2,Sink>, apart from the call
      * itself: 
      * \code
      * int main (...) 
      * {
      *   Source source;
      *   Pipe pipe;
      *   Sink sink;
      *
      *   // run one thread for the source, all available cores for the
      *   // pipe, and one thread for the sink.
      *   // In this example, both queues will be passing through items of the
      *   // same type, and items will be sent in batches of 1024 for both
      *   // queues.
      *   run_queue (
      *     source, 1, 
      *     Item(), 1024,
      *     pipe, 0,
      *     Item(), 1024, 
      *     sink, 1);
      * }
      * \endcode
      * 
      * \sa run_queue<Source,Type,Sink>
      * \sa run_batched_queue<Source,Type,Sink>
      * \sa run_batched_queue<Source,Type1,Pipe,Type2,Sink>
      */

    template <class Source, class Type1, class Pipe, class Type2, class Sink>
      inline void run_batched_queue (
          Source& source, size_t nthreads_source, 
          const Type1& item_type1, size_t batch_size1,
          Pipe& pipe, size_t nthreads_pipe,
          const Type2& item_type2, size_t batch_size2,
          Sink& sink, size_t nthreads_sink)
      {
        Queue<std::vector<Type1> > queue1 ("queue1");
        Queue<std::vector<Type2> > queue2 ("queue2");

        Batch::__Source<Type1,Source> q_source (queue1, source, batch_size1);
        Batch::__Pipe<Type1,Pipe,Type2> q_pipe (queue1, pipe, queue2, batch_size2);
        Batch::__Sink<Type2,Sink> q_sink (queue2, sink);

        Array<Batch::__Source<Type1,Source> > source_list (q_source, nthreads_source ? nthreads_source : available_cores());
        Array<Batch::__Pipe<Type1,Pipe,Type2> > pipe_list (q_pipe, nthreads_pipe ? nthreads_pipe : available_cores());
        Array<Batch::__Sink<Type2,Sink> > sink_list (q_sink, nthreads_sink ? nthreads_sink : available_cores());

        Exec source_threads (source_list, "source");
        Exec pipe_threads (pipe_list, "pipe");
        Exec sink_threads (sink_list, "sink");
      }


    /** @} */
  }
}

#endif


