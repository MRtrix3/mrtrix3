/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef __mrtrix_parfor_h__
#define __mrtrix_parfor_h__

#include <atomic>
#include <future>
#include <functional>

#include "thread.h"


namespace MR
{
  namespace Thread
  {

    /*! \brief Parallel for loop.
     *
     *  Minimalist parallel for loop, that distributes calls to functor fn
     *  across the number of threads set in MRtrix. The functor can be any
     *  callable object of type void(T), where T is the current loop index.
     *
     *  This implementation is inspired by the example in:
     *  http://www.andythomason.com/2016/08/21/c-multithreading-an-effective-parallel-for-loop/
     */
    template <typename T = size_t>
    void parallel_for(T begin, T end, std::function<void(T)> fn) {
      std::atomic<T> idx;
      idx = begin;

      size_t nthreads = number_of_threads();
      std::vector<std::future<void>> futures(nthreads);
      for (size_t thread = 0; thread < nthreads; ++thread) {
        futures[thread] = std::async(
          std::launch::async,
          [thread, &idx, end, &fn]() {
            for (;;) {
              T i = idx++;
              if (i >= end) break;
              fn(i);
            }
          }
        );
      }

      for (size_t thread = 0; thread < nthreads; ++thread) {
        futures[thread].get();
      }

    }


    /*! \brief Parallel sum.
     *
     *  Parallel sum, distributes across the number of threads set in MRtrix.
     *  Takes a functor of type void(T, R&), where T is the loop index and R
     *  is a reference to a thread-specific temporary sum. Functor fn should
     *  internally add its result to R&.
     *  R must be copy-constructable and must implement operator +=.
     *
     */
    template <typename R, typename T = size_t>
    R parallel_sum(T begin, T end, std::function<void(T, R&)> fn, const R& zero) {
      std::atomic<T> idx;
      idx = begin;

      size_t nthreads = number_of_threads();
      std::vector<std::future<void>> futures(nthreads);
      std::vector<R> results(nthreads, zero);

      for (size_t thread = 0; thread < nthreads; ++thread) {
        futures[thread] = std::async(
          std::launch::async,
          [thread, &idx, end, &fn, &results]() {
            for (;;) {
              T i = idx++;
              if (i >= end) break;
              fn(i, results[thread]);
            }
          }
        );
      }

      for (size_t thread = 0; thread < nthreads; ++thread) {
        futures[thread].get();
      }

      R result {zero};
      for (size_t thread = 0; thread < nthreads; ++thread) {
        result += results[thread];
      }

      return result;
    }


  }
}

#endif


