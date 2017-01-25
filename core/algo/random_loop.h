/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __algo_random_loop_h__
#define __algo_random_loop_h__

#include "apply.h"
#include "progressbar.h"
#include "stride.h"
#include "image_helpers.h"
#include <unordered_set>
#include <algorithm>  // std::shuffle
#include <iterator>

namespace MR
{

 template <class ImageType, class RandomEngine>
  class Random_loop {
    public:
      Random_loop (ImageType& in,
        RandomEngine& random_engine,
        const size_t& axis = 0,
        const size_t& number_iterations = std::numeric_limits<ssize_t>::max()):
          image (in),
          engine (random_engine),
          ax (axis),
          idx(image.size(axis)),
          max_cnt (number_iterations),
          status (true),
          cnt (0)
      {
        init();
        set_next_index();
      }
      void init() {
        std::iota (std::begin(idx), std::end(idx), 0);
        std::shuffle(std::begin(idx), std::end(idx), engine);
        cnt = 0;
        it = std::begin(idx);
        stop = std::end(idx);
      }

      void set_next_index() {
        ++cnt;
        image.index(ax) = *it;
        it++;
      }

      void operator++ ()
      {
        set_next_index();
        if (cnt > max_cnt or it == stop) {
          status = false;
        }
      }
      operator bool() const
      {
        return status && image.index(ax) >= 0 && image.index(ax) < image.size(ax);
      }

    private:
      ImageType& image;
      RandomEngine& engine;
      size_t ax;
      std::vector<size_t> idx;
      std::vector<size_t>::iterator it;
      std::vector<size_t>::iterator stop;
      size_t max_cnt;
      bool status;
      size_t cnt;
 };

  // Random_sparse_loop: ok for VERY sparse loops, slows down significantly at higher density (>5%)
  template <class ImageType>
    class Random_sparse_loop {
      public:
        Random_sparse_loop (ImageType& in,
          const size_t& axis = 0,
          const size_t& number_iterations = std::numeric_limits<ssize_t>::max(),
          const bool repeat = false,
          const ssize_t& min_index = 0,
          const ssize_t& max_index = std::numeric_limits<ssize_t>::max()):
            image (in),
            repeat_ (repeat),
            status (true),
            ax (axis),
            cnt (0),
            min_idx (min_index)
        {
          if (max_index < image.size(ax))
            range = max_index - min_idx + 1;
          else
            range = image.size(ax) - min_idx;
          if (number_iterations < range)
            max_cnt = number_iterations;
          else
            max_cnt = range;
          assert (range >= 1);
          assert (max_cnt >= 1);
          if (repeat_)
            set_next_index_with_repeat();
          else
            set_next_index_no_repeat();
        }

        void set_next_index_no_repeat() {
          while (cnt < max_cnt){
            index = rand() % range + min_idx;
            if (!idx_done.count(index)){
              idx_done.insert(index);
              break;
            }
          }
          ++cnt;
          image.index(ax) = index;
          assert(idx_done.size() < range);
        }

        void set_next_index_with_repeat() {
          index = rand() % range + min_idx;
          ++cnt;
          image.index(ax) = index;
        }

        void operator++ ()
        {
          if (repeat_)
            set_next_index_with_repeat();
          else
            set_next_index_no_repeat();
          if (cnt > max_cnt) {
            status = false;
          }
        }
        operator bool() const
        {
          return status && image.index(ax) >= 0 && image.index(ax) < image.size(ax);
        }

      private:
        ImageType& image;
        bool repeat_;
        bool status;
        size_t ax;
        size_t cnt;
        ssize_t min_idx;
        size_t range;
        size_t max_cnt;
        ssize_t index;
        std::unordered_set<ssize_t> idx_done;
   };

  template <class ImageType, class IterType>
    class Iterator_loop {
      public:
        Iterator_loop (ImageType& in,
          IterType first,
          IterType last,
          const size_t& axis = 0,
          const size_t& number_iterations = std::numeric_limits<ssize_t>::max()):
            image (in),
            ax (axis),
            start (first),
            stop (last),
            max_cnt (number_iterations),
            status (true),
            cnt (0)
        {
          set_next_index();
        }

        void set_next_index() {
          ++cnt;
          image.index(ax) = *start;
          start++;
        }

        void operator++ ()
        {
          set_next_index();
          if (cnt > max_cnt or start == stop) {
            status = false;
          }
        }
        operator bool() const
        {
          return status && image.index(ax) >= 0 && image.index(ax) < image.size(ax);
        }

      private:
        ImageType& image;
        size_t ax;
        IterType& start;
        IterType& stop;
        size_t max_cnt;
        bool status;
        size_t cnt;
   };


  //! @}
}

#endif


