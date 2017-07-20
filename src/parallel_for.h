/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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


  }
}

#endif


