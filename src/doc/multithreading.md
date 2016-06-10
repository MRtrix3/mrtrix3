Writing multi-threaded applications      {#multithreading}
===================================
   
Multi-threading allows applications to make use of all available CPU cores
on modern multi-core systems, providing an immediate performance boost.
MRtrix uses the C++11 threads API to enable multi-threaded processing where
possible. To facilitate development of multi-threaded applications, MRtrix
includes a number of classes that make it easy for many common workflows.


Overview      {#multithreading_overview}
========

In a nutshell, multi-threading refers to the concurrent execution of a number
of light-weight processes (threads) that share a common memory address space.
This makes it very easy for processes to process shared data, but also very
easy for one process to unexpectedly modify data that other threads are
currently operating on. The biggest challenge in multi-threaded programming is
managing concurrent access to shared resources and avoiding
[race conditions](http://en.wikipedia.org/wiki/Race_condition).
Concurrent execution can introduce strange and unexpected bugs, often difficult
to reproduce, and whose symptoms are often seemingly unrelated to the original
cause. This makes multi-threaded applications very difficult to debug. For this
reason, the best approach to developing multi-threaded applications is to keep
things simple and clearly organised, and to code with concurrency in mind -
even if the class you might be writing is not designed to be used in a
multi-threaded context now, it might make sense later.

The most common strategy for managing concurrent access is the concept of
[mutual exclusion](http://en.wikipedia.org/wiki/Mutual_exclusion>)
(a.k.a. mutex). Essentially, this involves identifying any shared resource that
threads will need to access, and programming access to this resource in such a
way that each thread is guaranteed sole access to the resource while it
operates on it. This essentially serialises access to the resource, ensuring
that each operation is completed before the next thread is allowed access. For
example, iterating through a set of data in a multi-threaded fashion might
involve each thread locking a mutex as it fetches the next item of data to be
processed from a queue. This ensures that no two threads end up with the same
data item to process, that no item accidentally remains unprocessed, etc. 

How to design a multi-threading application      {#multithreading_planning}
===========================================

Designing multi-threaded applications requires a different way of programming,
both to achieve a simple and robust solution that is not likely to crash, and
also to realise the expected performance gains. The single most important
aspect in the design process is how to structure the code to clearly define
which bits are 'owned' by each thread, which bits are shared across them, and
implementing strategies to enforce this design.



Sharing read-only data      {#multithreading_shared}
======================

The approach generally used in MRtrix applications is to define a class to hold
data that threads need read-only access to during processing. This class is
then initialised prior to creating and launching the threads, and is passed by
const-reference to the threads to prevent the threads from unintentionally
modifying the data. For example:

~~~{.cpp}
class Shared:
  public: 
    Shared () {
      // initialise, etc
      }

    SomeType shared_parameters;
    ...
};
~~~

This class might for example contain all the reconstruction parameters set by
the user, and any number of potentially large structures (e.g. matrices) that
will be used (but not modified) by the other threads. Once initialised, it is
then passed by const-reference to the thread class's constructor:

~~~{.cpp}
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
~~~

This structure helps to clearly delineate what is shared across threads and
what is private to each thread. Using this construct, each thread class has
read-only (const) access to the shared data, and any attempt at non-const
access to this data will result in a compile-time failure. This is a very good
thing, since trying to debug subtle bugs introduced by race conditions can be
very frustrating, and any strategy designed to catch unintended access will
save many hours of exasperation further down the track.


Sharing read/write data     {#multithreading_concurrent}
=======================

To allow thread-safe read/write access, the lowest-level approach is to design
an additional class that encapsulates the data to be protected. This class
provides methods to access data items, and manages the mutex associated with
the data. For example:

~~~{.cpp}
class SharedRW {
  public:
    ...

    Item get_next () {
      // note: this is pseudo-code!
      lock_mutex();
      Item item = list.pop();
      unlock_mutex();
      return item;
    }

    ...
};
~~~

In this example, access to the list is protected by locking and releasing a
mutex before and after reading the next item from the list, ensuring no two
threads can modify the list's internal data structures concurrently (note the
`lock_mutex()` & `unlock_mutex()` functions used in this example are for
illustration purposes only - no such functions exist within MRtrix).


Achieving maximum performance     {#multithreading_performance}
=============================

Ideally, using multi-threading would provide a speedup factor equivalent to the
number of cores used. In practice, a number of issues might prevent this from
happening. Some of these are hardware-related (e.g. hyper-threading won't give
all the performance benefits of a genuine multi-core equivalent CPU), some of
them are due to restrictions imposed by the algorithm to be implemented, and
some are caused by poor design of the implementation. While the first two are
typically unavoidable, there is a lot that can be done for the latter. 

The first issue to consider is the overhead of the multi-threading calls.  Each
call to lock or release a mutex will take up CPU cycles due to the function
call and the need for the hardware to synchronise across all CPU cores. This
overhead can become significant if run within a sufficiently tight loop, where
the processing itself might take up fewer CPU cycles than the mutex handling. 

Another related issue is the cost of holding the mutex, in terms of its impact
on other threads. If the mutex is often locked, threads will spend a
significant proportion of their time waiting for it to be released. This is
particularly the case if the amount of processing done while the lock is held
is large in relation to the amount of work done while it is not held.

These considerations suggest that optimal performance is achieved by keeping
the rate of mutex locking low, and keeping the amount of work done while the
lock is held small. This can often be done using simple strategies such as
passing pointers to data to be processed, so that pushing or pulling them from
a mutex-protected list is quick. If the operation to be performed per item is
inherenly small, these can be done in batches to reduce the amount of locking. 

There are many other issues that can impact performance, such as non-optimal
memory access. If each thread is trying to access large amounts of data in
seemingly random locations spread all over the RAM, the CPU will probably end
up spending a disproportionate amount of time fetching data and flushing the
CPU cache, resulting in poor performance. When this happens, multi-threading
may seem to provide no benefit, and might in fact adversely affect performance
compared to the single-threaded equivalent. It helps to think hard about your
code's memory access patterns to take advantage of the CPU's onboard cache and
the high burst transfer rates that can be achieved when the CPU can read large
chunks of contiguous data. This is even more important when operating on
disk-backed data, where the cost of non-linear access can be considerable.


Multi-threading API in MRtrix     {#multithreading_in_mrtrix}
=============================

MRtrix provides a number of constructs to simplify the process of writing solid
multi-threading applications. In most cases, the high-level 
@ref image_thread_looping and @ref thread_queue frameworks will be appropriate
for the particular algorithm to be implemented. If a more sophisticated
implementation is required, the C++11 threads API can be used directly.



Launching threads      {#multithreading_exec}
=================

The MR::Thread::run() function is designed to launch and manage one or more
threads. This is done by providing one or more specially-designed thread
classes (or functors), whose `execute()` method will be invoked within the
newly-created thread. Multiple instances of the same thread class can be
launched by wrapping the functor within the `MR::Thread::multi()` call when
passing it to MR::Thread::run().

@note If the class is to be used in multiple concurrent threads (i.e.
launched using MR::Thread::multi()), the class must be copy-constructable,
and any copy created in this way must be fully independent: if pointers to
non-const data or other complex structures are members of the class, there is a
good chance the default copy constructor will provide a copy referencing the
same data as the original, which would generally lead to race conditions if
these were written to during processing. Most of the classes provided in MRtrix
do copy-construct appropriately for use in multi-threading (for example
[Image], [Adapter]), while others do not). That said, if all class members have
appropriate copy constructors, the default copy constructor should also be
appropriate.

For example:

~~~{.cpp}
class MyThread {
  public:
    void execute() {
      // do stuff
    }
};


void run() {
  MyThread thread;
  auto exec = Thread::run (thread, "my thread");

  ...
  // do something else
  ...

  // invoking thread will wait for other thread to finish (join)
  // in the thread class's destructor - i.e. when thread goes out of scope 
}
~~~

To run it across multiple threads, use the MR::Thread::multi() wrapper. By
default, the number of threads launched is determined from the C++11
`std::thread::hardware_concurrency()` function, from the config file or from the
command line.

~~~{.cpp}
void run () {
  MyThread thread;

  auto exec = Thread::run (Thread::multi (thread), "my threads");

  ...
  // do something else, or just wait for completion.
  ...

  // invoking thread will wait for all other threads to finish (join)
  // in thread class's destructor - i.e. when exec goes out of scope 
}
~~~



The ThreadedLoop           {#multithreading_loop}
================

It is very common in imaging to process image voxels. The MR::ThreadedLoop()
function and associated classes are designed to greatly simplify the process of
creating such applications. It allows the developer to provide a simple functor
class implementing the operation to be performed for each voxel, which can be
passed to MR::ThreadedLoop() to be executed over the whole dataset. Refer to
@ref image_thread_looping documentation for more information.




The Thread::Queue        {#multithreading_queue}
=================

In many cases, an algorithm can be designed to process a stream of data.  For
example, data items are read in order from disk, and can be processed
independently. The results might then need to be written back to disk in a
serial fashion. The MR::Thread::Queue class is designed to facilitate this type
of operation. Please refer to the @ref thread_queue for a detailed description. 

There is also a convenience function to simplify the setting up of
the queue structure, called MR::Thread::run_queue(). This also works with the
MR::Thread::multi() wrapper where multiple parallel threads are desired. 
Please refer to the [Thread module documentation[(@ref thread_classes) for details.


