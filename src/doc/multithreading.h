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
    on modern multi-core system, providing an immediate performance boost.
    MRtrix uses the <a href=http://en.wikipedia.org/wiki/POSIX_Threads>POSIX
    threads</a> API to enable multi-threaded processing where possible. To
    facilitate development of multi-threaded applications, MRtrix includes a
    number of classes that make it easy for many common workflows.

    \section multithreading_overview Overview

    In a nutshell, multi-threading refers to the concurrent execution of a
    number of light-weight processes (threads) that share a common memory
    address space.  This makes both very easy for processes to exchange data,
    but also very easy for one process to unexpectedly modify data that other
    threads are currently operating on. The biggest challenge in multi-threaded
    programming is managing concurrent access to shared resources and avoiding
    <a href=http://en.wikipedia.org/wiki/Race_condition>race conditions</a>. 

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
    code to clearly define which bits are 'owned' by each thread, and which
    bits are shared across them. 
    
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
    initialised, it is then passed by const-reference to the thread classes'
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
    threads can modify the list's internal data structures concurrently. 



    \section multithreading_posix Simple POSIX wrappers

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
        MyThread () {
          // initialise
        }
    \endcodce

   */

}


