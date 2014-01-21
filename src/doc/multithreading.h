/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/08/09.

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

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR
{

  /*! \page multithreading Writing multi-threaded applications
   
    Multi-threading allows applications to make use of all available CPU cores
    on modern multi-core systems, providing an immediate performance boost.
    MRtrix uses the <a href=http://en.wikipedia.org/wiki/POSIX_Threads>POSIX
    threads</a> API to enable multi-threaded processing where possible. To
    facilitate development of multi-threaded applications, MRtrix includes a
    number of classes that make it easy for many common workflows.

    \section multithreading_overview Overview

    In a nutshell, multi-threading refers to the concurrent execution of a
    number of light-weight processes (threads) that share a common memory
    address space.  This makes it very easy for processes to process shared
    data, but also very easy for one process to unexpectedly modify data that
    other threads are currently operating on. The biggest challenge in
    multi-threaded programming is managing concurrent access to shared
    resources and avoiding <a
    href=http://en.wikipedia.org/wiki/Race_condition>race conditions</a>.
    Concurrent execution can introduce strange and unexpected bugs, often
    difficult to reproduce, and whose symptoms are often seemingly unrelated to
    the original cause. This makes multi-threaded applications very difficult
    to debug. For this reason, the best approach to developing multi-threaded
    applications is to keep things simple and clearly organised, and to code
    with concurrency in mind - even if the class you might be writing is not
    designed to be used in a multi-threaded context now, it might make sense
    later.

    The most common strategy for managing concurrent access is the concept of
    <a href=http://en.wikipedia.org/wiki/Mutual_exclusion>mutual exclusion</a>
    (a.k.a mutex). Essentially, this involves identifying any shared resource
    that threads will need to access, and programming access to this resource
    in such a way that each thread is guaranteed sole access to the resource
    while it operates on it. This essentially serialises access to the
    resource, ensuring that each operation is completed before the next thread
    is allowed access. For example, iterating through a set of data in a
    multi-threaded fashion might involve each thread locking a mutex as it
    fetches the next item of data to be processed from a queue. This ensures
    that no two threads end up with the same data item to process, that no item
    accidentally remains unprocessed, etc. 

    \section multithreading_planning How to design a multi-threading application

    Designing multi-threaded applications requires a different way of
    programming, both to achieve a simple and robust solution that is not
    likely to crash, and also to realise the expected performance gains. The
    single most important aspect in the design process is how to structure the
    code to clearly define which bits are 'owned' by each thread, which
    bits are shared across them, and implementing strategies to enforce this
    design.
    
    \subsection multithreading_shared Sharing read-only data

    The approach generally used in MRtrix applications is to define a class to
    be shared across threads, called for example \c Shared:
    \code 
    class Shared:
      public: 
        Shared () {
          // initialise, etc
          }

        SomeType shared_parameters;
        ...
    };
    \endcode

    This class could for example contain all the reconstruction parameters set
    by the user, and any number of potentially large structures (e.g. matrices)
    that will be used by the other threads in a read-only fashion. Once
    initialised, it is then passed by const-reference to the thread class's
    constructor:
    \code
    class Thread {
      public:
        Thread (const Shared& shared) :
          shared (shared) {
            // any per-thread initialisation
          }

      protected:
        const Shared& shared;
        OtherType local_variables;
    };
    \endcode
    
    At this point, each thread class has access to the shared data in a
    read-only (const) and hopefully thread-safe fashion. 

    This structure helps to clearly delineate what is shared across threads and
    what is private to each thread. Any attempt at non-const access to the
    shared data will result in a compile-time failure. This is a very good
    thing, since trying to debug subtle bugs introduced by race conditions can
    be very frustrating.

    \subsection multithreading_concurrent Sharing read/write data

    To allow thread-safe read/write access, the lowest-level approach is to
    design an additional class that encapsulates the data to be protected. This
    class provides methods to access data items, and manages the mutex
    associated with the data. For example:
    \code
    class SharedRW {
      public:
        SharedRW () {
          // initialise 
        }

        Item get_next () {
          lock_mutex();
          Item item = list.pop();
          unlock_mutex();
          return item;
        }

      protected:
        ListType list;
    };
    \endcode

    In this example, access to the list is protected by locking and releasing a
    mutex before and after reading the next item from the list, ensuring no two
    threads can modify the list's internal data structures concurrently (note
    the lock_mutex() & unlock_mutex() functions used in this example are for
    illustration purposes only - no such functions exist within MRtrix).


    \subsection multithreading_performance Achieving maximum performance

    Ideally, using multi-threading would provide a speedup factor equivalent to
    the number of cores used. In practice, a number of issues might prevent
    this from happening. Some of these are hardware-related (e.g.
    hyper-threading won't give all the performance benefits of a genuine
    multi-core equivalent CPU), some of them are due to restrictions
    imposed by the algorithm to be implemented, and some are caused by poor
    design of the implementation. While the first two are typically
    unavoidable, there is a lot that can be done for the latter. 

    The first issue to consider is the overhead of the multi-threading calls.
    Each call to lock or release a mutex will take up CPU cycles due to the
    function call and the need for the hardware to synchronise across all CPU
    cores. This overhead can become significant if run within a sufficiently
    tight loop, where the processing itself might take up fewer CPU cycles that
    the mutex handling. 

    Another related issue is the cost of holding the mutex, in terms of its
    impact on other threads. If the mutex is often locked, threads will spend a
    significant proportion of their time waiting for it to be released. This is
    particularly the case if the amount of processing done while the lock is
    held is large in relation to the amount of work done while it is not held.

    These considerations suggest that optimal performance is achieved by keeping
    the rate of mutex locking low, and keeping the amount of work done while
    the lock is held small. This can often be done using simple strategies such
    as passing pointers to data to be processed, so that pushing or pulling
    them from a mutex-protected list is quick. If the operation to be performed
    per item is inherenly small, these can be done in batches to reduce the
    amount of locking. 

    There are many other issues that can impact performance, such as
    non-optimal memory access. If each thread is trying to access large amounts
    of data in seemingly random locations spread all over the RAM, the CPU will
    probably end up spending a disproportionate amount of time fetching data
    and flushing the CPU cache, resulting in poor performance. When this
    happens, multi-threading may seem to provide no benefit, and might in fact
    adversely affect performance compared to the single-threaded equivalent. It
    helps to think hard about your code's memory access patterns to take
    advantage of the CPU's onboard cache and the high burst transfer rates that
    can be achieved when the CPU can read large chunks of contiguous data. This
    is even more important when operating on disk-backed data, where the cost
    of non-linear access can be considerable.


    \section multithreading_in_mrtrix Multi-threading API in MRtrix


    \subsection multithreading_posix Simple POSIX wrappers

    MRtrix provides a set of classes encapsulating POSIX basic objects:
    Thread::Mutex (and its associated Thread::Mutex::Lock), and Thread::Cond.
    These are essentially thin wrappers around the equivalent POSIX objects. To
    invoke threads and wait on their completion, the classes Thread::Exec (for
    a single thread) and Thread::Array (for a set of identical threads) are
    provided.

    For example:
    \code
    class MyThread {
      public:
        MyThread (ListType& list) :
          list (list) {
          // initialise
        }

        void execute () { // this is the method that will be executed in the thread
          Item* item;
          while ((item = get_next()) != NULL) {
            // process item
          }
        }

        Item* get_next() {
          Thread::Mutex::Lock lock (mutex); // locks the mutex
          return list.pop();
        } // mutex is released when lock goes out of scope

      protected:
        ListType& list;
        Thread::Mutex mutex;
        ...
    };
    \endcode

    The thread class's execute() method can be run in one or more different threads using Thread::Exec, for example:
    \code
    ListType list;
    // initialise list
    
    void run () {
      MyThread thread (list);
      Thread::Exec exec (thread, "my thread");

      ...
      // do something else
      ...

      // invoking thread will wait for other thread to finish (join)
      // in Thread::Exec's destructor - i.e. when exec goes out of scope 
    }
    \endcode

    To run it across multiple threads, use the Thread::Array class:
    \code
    void run () {
      MyThread thread (list);
      // by default, the number of threads launched is set in the config file
      // or on the command line.
      Thread::Array thread_array (thread);
      Thread::Exec exec (thread_array, "my threads");

      ...
      // do something else, or just wait for completion.
      ...

      // invoking thread will wait for all other threads to finish (join)
      // in Thread::Exec's destructor - i.e. when exec goes out of scope 
    }
    \endcode

    \note For these classes to work as intended, the thread class provided by the
    developer must meet a few requirements. First, it must provide an execute()
    method, which will be invoked by Thread::Exec. Second, if the class is to be
    used in Thread::Array, the class must be copy-constructable, and any copy
    created in this way must be fully independent: if pointers to non-const data
    or other complex structures are members of the class, there is a good chance
    the default copy constructor will provide a copy referencing the same data as
    the original, which would generally lead to race conditions if these were
    written to during processing. Most of the classes provided in MRtrix do
    copy-construct appropriately for use in multi-threading (for example
    Image::Voxel, Math::Vector, Math::Matrix, Ptr), while others do not
    (Image::Buffer for example). That said, if all class members have
    appropriate copy constructors, the default copy constructor should also be
    appropriate.


    \subsection multithreading_queue The Thread::Queue

    In many cases, an algorithm can be designed to process a stream of data.
    For example, data items are read in order from disk, and can be processed
    independently. The results might then need to be written back to disk in a
    serial fashion. The Thread::Queue class is designed to facilitate this type
    of operation. Please refer to the Thread::Queue documentation for a
    detailed description.


    \subsection multithreading_loop The Image::ThreadedLoop 

    It is even more common in imaging to process image voxels. The
    Image::ThreadedLoop class is designed to greatly simplify the process of
    creating such applications. It allows the developer to provide a simple
    functor class implementing the operation to be performed for each voxel,
    which can be passed to Image::ThreadedLoop to be executed over the whole
    dataset. Refer to the Image::ThreadedLoop documentation for more
    information.



   */

}


