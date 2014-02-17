#ifndef __mrtrix_thread_mutex_h__
#define __mrtrix_thread_mutex_h__

#include <pthread.h>

namespace MR
{
  namespace Thread
  {

    /** \addtogroup thread_basics
     * @{ */

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
    class Mutex
    {
      public:
        Mutex () {
          pthread_mutex_init (&_mutex, NULL);
        }
        ~Mutex () {
          pthread_mutex_destroy (&_mutex);
        }

        void lock () {
          pthread_mutex_lock (&_mutex);
        }
        void unlock () {
          pthread_mutex_unlock (&_mutex);
        }

        class Lock
        {
          public:
            Lock (Mutex& mutex) : m (mutex) {
              m.lock();
            }
            ~Lock () {
              m.unlock();
            }
          private:
            Mutex& m;
        };
      private:
        pthread_mutex_t _mutex;
        friend class Cond;
    };

    /** @} */
  }
}

#endif


