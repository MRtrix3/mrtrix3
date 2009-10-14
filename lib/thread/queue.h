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

#include "thread/condition.h"

namespace MR {
  namespace Thread {

    /** \addtogroup Thread 
     * @{ */

    //! The default allocator used in the Thread::Queue
    /*! This is the default allocator used to allocate, deallocate and reset
     * items for use in a Thread::Queue. To use non-default constructors,
     * destructors, or to ensure items are reset to a default state before
     * being re-used, a custom allocator class should be used. This is
     * illustrated in the example below:
     * \code
     * // An item class that has a non-default constructor, 
     * // and a method to reset the item to a standard state:
     * class Item {
     *   public:
     *     Item (size_t number_of_elements);
     *     ~Item ();
     *     void clear ();
     *     ...
     * };
     *
     *
     *
     * // An allocator class designed to operate with the Item:
     * class ItemAllocator {
     *   public:
     *     ItemAllocator (size_t number_of_elements) : N (number_of_elements) { }
     *     Item* alloc () { return (new Item (N)); }
     *     void reset (Item* p) { p->clear(); }
     *     void dealloc (Item* p) { delete p; }
     *   private:
     *     size_t N;
     * };
     *
     *
     * void my_function () {
     *   // Supply this allocator to the Queue at construction:
     *   Thread::Queue<Item,ItemAllocator> queue ("my queue", 100, ItemAllocator (number_required());
     *   
     *   ...
     *   // Set up Functors, threads, etc, and use as normal
     *   ...
     * }
     * \endcode
     *
     * \note The allocator will be passed to the Queue by copy, using the copy
     * constructor. If using a non-standard allocator, you need to ensure that
     * the copy constructor is valid and will produce a functional allocator.
     */
    template <class T> class DefaultAllocator {
      public:
        //! Allocate a new item
        /*! Return a pointer to a newly allocated item of type \c T. 
         *
         * By default, this simply returns a pointer to a new item constructed
         * using the new() operator and the default constructor. */

        T* alloc () { return (new T); }
        //! reset the item for re-use
        /*! This should ensure the item is returned to the expected state for
         * re-use. 
         *
         * By default, it does nothing. */
        void reset (T* p) { }

        //! Deallocate the item
        /*! This should free all memory associated with the item.
         *
         * By default, it simply calls the delete() operator. */
        void dealloc (T* p) { delete p; }
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
     *   classes are designed to be instanciated within each functor's
     *   execute() method. They must be constructed from a reference to a
     *   Thread::Queue::Writer or Thread::Queue::Reader respectively, ensuring
     *   no reads or write can take place without having registered with the
     *   queue. Their destructors will also unregister from the queue, ensuring
     *   that each thread unregisters as soon as the execute() method returns,
     *   and hence \e before the thread exits.
     *
     * The Queue class performs all memory management for the items in the
     * queue. For this reason, the items are accessed via the Writer::Item &
     * Reader::Item classes, and constructed using the Allocator class passed
     * as a template parameter for the queue. This allows items to be recycled
     * once they have been processed, reducing overheads associated with memory
     * allocation/deallocation. If you need the items to be constructed using a
     * non-default constructor, then you will need to create your own
     * allocator. See Thread::DefaultAllocator for details.
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
    template <class T, class Allocator = DefaultAllocator<T> > class Queue {
      public:
        //! Construct a Queue of items of type \c T
        /*! \param description a string identifying the queue for degugging purposes
         * \param buffer_size the maximum number of items that can be pushed onto the queue before
         * blocking. If a thread attempts to push more data onto the queue when the
         * queue already contains this number of items, the thread will block until
         * at least one item has been popped.  By default, the buffer size is 100
         * items.
         * \param alloc an allocator object, which can be supplied to handle
         * non-standard item construction, destruction, or reset operation. See
         * Thread::DefaultAllocator for details.
         */
        Queue (const std::string& description = "unnamed", size_t buffer_size = 100, const Allocator& alloc = Allocator()) : 
          allocator (alloc),
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
         * process of writing items to the queue is done via the Writer::Item
         * class.
         * 
         * See Thread::Queue for more information and an example. */
        class Writer {
          public: 
            //! Register a Writer object with the queue
            /*! The Writer object will register itself with the queue as a
             * writer. */
            Writer (Queue<T,Allocator>& queue) : Q (queue) { Q.register_writer(); }
            Writer (const Writer& W) : Q (W.Q) { Q.register_writer(); }

            //! This class is used to write items to the queue
            /*! Items cannot be written directly onto a Thread::Queue queue. An
             * object of this class must be instanciated and used to write to the
             * queue. 
             * 
             * See Thread::Queue for more information and an example. */
            class Item {
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
                ~Item () { Q.unregister_writer(); }
                //! Push the item onto the queue
                bool write () { return (Q.push (p)); }
                T& operator*() const throw ()   { return (*p); }
                T* operator->() const throw ()  { return (p); }
              private:
                Queue<T,Allocator>& Q;
                T* p;
            };

          private:
            Queue<T,Allocator>& Q;
        };


        //! This class is used to register a reader with the queue
        /*! Items cannot be read directly from a Thread::Queue queue. An
         * object of this class must be instanciated to notify the queue
         * that a section of code will be reading from the queue. The actual
         * process of reading items from the queue is done via the Reader::Item
         * class.
         * 
         * See Thread::Queue for more information and an example. */
        class Reader {
          public: 
            //! Register a Reader object with the queue.
            /*! The Reader object will register itself with the queue as a
             * reader. */
            Reader (Queue<T,Allocator>& queue) : Q (queue) { Q.register_reader(); }
            Reader (const Reader& reader) : Q (reader.Q) { Q.register_reader(); }

            //! This class is used to read items from the queue
            /*! Items cannot be read directly from a Thread::Queue queue. An
             * object of this class must be instanciated and used to read from the
             * queue.
             * 
             * See Thread::Queue for more information and an example. */
            class Item {
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
                ~Item () { Q.unregister_reader(); }
                //! Get next item from the queue
                bool read () { return (Q.pop (p)); }
                T& operator*() const throw ()   { return (*p); }
                T* operator->() const throw ()  { return (p); }
              private:
                Queue<T,Allocator>& Q;
                T* p;
            };
          private:
            Queue<T,Allocator>& Q;
        };

        //! Print out a status report for debugging purposes
        void status () { 
          Mutex::Lock lock (mutex);
          std::cerr << "Thread::Queue \"" + name + "\": " 
            << writer_count << " writer" << (writer_count > 1 ? "s" : "" ) << ", " 
            << reader_count << " reader" << (reader_count > 1 ? "s" : "" ) << ", items waiting: " << size() << "\n";
        }


      private:
        Allocator allocator;
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

        Queue (const Queue& queue) { assert (0); }
        Queue& operator= (const Queue& queue) { assert (0); return (*this); }

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

        T* get_item () { 
          Mutex::Lock lock (mutex); 
          T* item = allocator.alloc();
          items.push_back(item);
          return (item);
        }

        bool push (T*& item) { 
          Mutex::Lock lock (mutex); 
          while (full() && reader_count) more_space.wait();
          if (!reader_count) return (false);
          *back = item;
          back = inc (back);
          more_data.signal();
          if (item_stack.empty()) { item = allocator.alloc(); items.push_back(item); } 
          else { item = item_stack.top(); item_stack.pop(); }
          return (true);
        }

        bool pop (T*& item) {
          Mutex::Lock lock (mutex);
          if (item) { allocator.reset (item); item_stack.push (item); }
          item = NULL;
          while (empty() && writer_count) more_data.wait();
          if (empty() && !writer_count) return (false);
          item = *front;
          front = inc (front);
          more_space.signal();
          return (true);
        }

        T** inc (T** p) const { ++p; if (p >= buffer + capacity) p = buffer; return (p); }
    };


    /** @} */
  }
}

#endif


