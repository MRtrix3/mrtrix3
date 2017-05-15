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


#ifndef __algo_neighbourhooditerator_h__
#define __algo_neighbourhooditerator_h__

#include <vector>
#include "algo/iterator.h"
#include "types.h"

namespace MR
{

  /** \defgroup loop Looping functions
    @{ */

  //! a dummy image to iterate over a certain neighbourhood, useful for multi-threaded looping.
  // Does not work properly with Loop() functions! Use instead:
  //
  // vector extent(iter.ndim(),3) // max number of voxels to iterate over
  // auto niter = NeighbourhoodIterator(iter, extent);
  // while(niter.loop()){
  //   std::cerr << niter << std::endl;
  // }
  //
  //
  class NeighbourhoodIterator
  { MEMALIGN(NeighbourhoodIterator)
    public:
      NeighbourhoodIterator() = delete;
      template <class IteratorType>
        NeighbourhoodIterator (const IteratorType& iter, const vector<size_t>& extent) :
          dim (iter.ndim()),
          offset (iter.ndim()),
          // pos (iter.ndim()),
          pos_orig (iter.ndim()),
          ext (container_cast<decltype(ext)>(extent)),
          has_next_(false) {
            assert (iter.ndim() == extent.size());
            pos.resize(iter.ndim());
            for (size_t i = 0; i < iter.ndim(); ++i){
              ext[i] = (ext[i]-1) / 2;
              offset[i] = iter.index(i);
              // set pos to lower bound
              pos[i] = (offset[i] - ext[i] < 0) ? 0 : offset[i] - ext[i]; 
              pos_orig[i] = pos[i];
              // upper bound:
              auto high = (offset[i] + ext[i] >= iter.size(i)) ? iter.size(i) - 1 : offset[i] + ext[i]; 
              dim[i] = high - pos[i] + 1;
            }
          }

      size_t ndim () const { return dim.size(); }
      ssize_t size (size_t axis) const { return dim[axis]; }

      const ssize_t& index (size_t axis) const { return pos[axis]; }
      ssize_t& index (size_t axis) { return pos[axis]; }

      const Eigen::Matrix< ssize_t, 1, Eigen::Dynamic >  get_pos() const { return pos;  }

      const ssize_t& extent (size_t axis) const { return dim[axis]; }
      const ssize_t& centre (size_t axis) const { return offset[axis]; }

      void reset (size_t axis) { pos[axis] = pos_orig[axis]; }

      bool loop () {
        if (not this->has_next_){
          this->has_next_ = true;
          for (auto axis = dim.size(); axis-- != 0; )
            this->reset(axis);
          return true;
        }
        for (auto axis = dim.size(); axis-- != 0; ) { // TODO loop in stride order
          ++pos[axis];
          if (pos[axis] != (pos_orig[axis] + dim[axis]))
            return true;  
          this->reset(axis);
        }
        this->has_next_ = false;
        return false;
      }

      friend std::ostream& operator<< (std::ostream& stream, const NeighbourhoodIterator& V) {
        stream << "neighbourhood iterator, position [ ";
        for (size_t n = 0; n < V.ndim(); ++n)
          stream << V.index(n) << " ";
        stream << "], extent [ ";
        for (size_t n = 0; n < V.ndim(); ++n)
          stream << V.extent(n) << " ";
        stream << "], centre [ ";
        for (size_t n = 0; n < V.ndim(); ++n)
          stream << V.centre(n) << " ";
        stream << "]";
        return stream;
      }


    private:
      vector<ssize_t> dim, offset, pos_orig, ext;
      Eigen::Matrix< ssize_t, 1, Eigen::Dynamic > pos;
      bool has_next_;

      void value () const { assert (0); }
  };

  //! @}
}

#endif



