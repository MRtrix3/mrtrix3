/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/SIFT/gradient_sort.h"

#include <algorithm>

#include "thread_queue.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {





      MT_gradient_vector_sorter::MT_gradient_vector_sorter (MT_gradient_vector_sorter::VecType& in, const track_t block_size)
      {
        BlockSender source (in.size(), block_size);
        Sorter      pipe   (in);
        Thread::run_queue (source, TrackIndexRange(), Thread::multi (pipe), VecItType(), *this);
      }




      MT_gradient_vector_sorter::VecItType MT_gradient_vector_sorter::get()
      {
        VecItType return_iterator (*candidates.begin());
        VecItType incremented (*candidates.begin());
        ++incremented;
        candidates.erase (candidates.begin());
        candidates.insert (incremented);
        return return_iterator;
      }



      bool MT_gradient_vector_sorter::Sorter::operator() (const TrackIndexRange& in, VecItType& out) const
      {
        VecItType start      (data.begin() + in.first);
        VecItType from_end   (data.begin() + in.second);
        VecItType from_start (start);
        for (; from_start < from_end; ++from_start) {
          if (from_start->get_gradient_per_unit_length() >= 0.0) {
            for (--from_end; from_end > from_start && from_end->get_gradient_per_unit_length() >= 0.0; --from_end);
            std::swap (*from_start, *from_end);
          }
        }
        std::sort (start, from_start);
        out = start;
        return true;
      }






      }
    }
  }
}


