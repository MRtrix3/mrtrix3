#ifndef __mrtrix_thread_condition_h__
#define __mrtrix_thread_condition_h__

#include "thread/mutex.h"

namespace MR
{
  namespace Thread
  {

    /** \addtogroup thread_basics
     * @{ */

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
    class Cond
    {
      public:
        Cond (Mutex& mutex) : _mutex (mutex) {
          pthread_cond_init (&_cond, NULL);
        }
        ~Cond () {
          pthread_cond_destroy (&_cond);
        }

        //! wait until condition is reached
        void wait () {
          pthread_cond_wait (&_cond, &_mutex._mutex);
        }
        //! condition is reached: wake up at least one waiting thread
        void signal () {
          pthread_cond_signal (&_cond);
        }
        //! condition is reached: wake up all waiting threads
        void broadcast () {
          pthread_cond_broadcast (&_cond);
        }

      private:
        pthread_cond_t _cond;
        Mutex& _mutex;
    };

    /** @} */
  }
}

#endif


