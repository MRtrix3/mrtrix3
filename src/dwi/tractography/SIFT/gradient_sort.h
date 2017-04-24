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


#ifndef __dwi_tractography_sift_sort_h__
#define __dwi_tractography_sift_sort_h__


#include <set>
#include <vector>

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {




      class Cost_fn_gradient_sort
      { MEMALIGN(Cost_fn_gradient_sort)
        public:
          Cost_fn_gradient_sort (const track_t i, const double g, const double gpul) :
            tck_index            (i),
            cost_gradient        (g),
            grad_per_unit_length (gpul) { }

          Cost_fn_gradient_sort (const Cost_fn_gradient_sort& that) :
            tck_index            (that.tck_index),
            cost_gradient        (that.cost_gradient),
            grad_per_unit_length (that.grad_per_unit_length) { }

          void set (const track_t i, const double g, const double gpul) { tck_index = i; cost_gradient = g; grad_per_unit_length = gpul; }

          bool operator< (const Cost_fn_gradient_sort& that) const { return grad_per_unit_length < that.grad_per_unit_length; }

          track_t get_tck_index()                const { return tck_index; }
          double  get_cost_gradient()            const { return cost_gradient; }
          double  get_gradient_per_unit_length() const { return grad_per_unit_length; }

        private:
          track_t tck_index;
          double  cost_gradient;
          double  grad_per_unit_length;
      };




      // Sorting of the gradient vector in SIFT is done in a multi-threaded fashion, in a number of stages:
      // * Gradient vector is split into blocks of equal size
      // * Within each block:
      //     - Non-negative gradients are pushed to the end of the block (no need to sort these)
      //     - Negative gradients within the block are sorted fully
      //     - An iterator to the first entry in the sorted block is written to a set
      // * For streamline filtering, the candidate streamline is chosen from the beginning of this set.
      //     The entry in the set corresponding to the gradient vector is removed, but the iterator WITHIN THE BLOCK
      //     is incremented and re-written to the set; this allows multiple streamlines from a single block to
      //     be filtered in a single iteration, provided the gradient is less than that of the candidate streamline
      //     from all other blocks
      class MT_gradient_vector_sorter
      { MEMALIGN(MT_gradient_vector_sorter)

          using VecType = vector<Cost_fn_gradient_sort>;
          using VecItType = VecType::iterator;

          class Comparator { NOMEMALIGN
            public:
              bool operator() (const VecItType& a, const VecItType& b) const { return (a->get_gradient_per_unit_length() < b->get_gradient_per_unit_length()); }
          };
          using SetType = std::set<VecItType, Comparator>;


        public:
          MT_gradient_vector_sorter (VecType& in, const track_t);

          VecItType get();

          bool operator() (const VecItType& in)
          {
            candidates.insert (in);
            return true;
          }


        private:
          SetType candidates;


          class BlockSender
          { MEMALIGN(BlockSender)
            public:
              BlockSender (const track_t count, const track_t size) :
                num_tracks (count),
                block_size (size),
                counter (0) { }
              bool operator() (TrackIndexRange& out)
              {
                if (counter == num_tracks) {
                  out.first = out.second = 0;
                  return false;
                }
                out.first = counter;
                counter = std::min (counter + block_size, num_tracks);
                out.second = counter;
                return true;
              }
            private:
              const track_t num_tracks, block_size;
              track_t counter;
          };

          class Sorter
          { MEMALIGN(Sorter)
            public:
              Sorter (VecType& in) :
                data  (in) { }
              Sorter (const Sorter& that) :
                data  (that.data) { }
              bool operator() (const TrackIndexRange&, VecItType&) const;
            private:
              VecType& data;
          };


      };




      }
    }
  }
}


#endif


