/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 16/10/09.

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

#ifndef __algo_neighbourhooditerator_h__
#define __algo_neighbourhooditerator_h__

#include <vector>
#include "algo/iterator.h"
#include "types.h"

namespace MR
{

  /** \defgroup loop Looping functions
    @{ */

  //! a dummy image to iterate over, useful for multi-threaded looping.
  // namespace {
  //   struct set_pos {
  //     FORCE_INLINE set_pos (size_t axis, ssize_t index) : axis (axis), index (index) { }
  //     template <class ImageType> 
  //       FORCE_INLINE void operator() (ImageType& vox) { vox.index(axis) = index+offset[i]; }
  //     size_t axis;
  //     ssize_t index;
  //   };
  // }
  class NeighbourhoodIterator
  {
    public:
      NeighbourhoodIterator() = delete;
      template <class IteratorType>
        NeighbourhoodIterator (const IteratorType& iter, const std::vector<size_t>& extent) :
          dim (iter.ndim()),
          offset (iter.ndim()),
          pos (iter.ndim()),
          ext (container_cast<decltype(ext)>(extent)) {
            assert (iter.ndim() == extent.size());
            for (size_t i = 0; i < iter.ndim(); ++i){
              ext[i] = (ext[i]-1) / 2;
              offset[i] = iter.index(i);
              // set pos to lower bound
              pos[i] = (offset[i] - ext[i] < 0) ? 0 : offset[i] - ext[i]; 
              // upper bound:
              auto high = (offset[i] + ext[i] >= iter.size(i)) ? iter.size(i) - 1 : offset[i] + ext[i]; 
              dim[i] = high - pos[i] + 1;
            }
            VAR(ext);
            VAR(pos);
            VAR(dim);
          }

      size_t ndim () const { return dim.size(); }
      ssize_t size (size_t axis) const { return dim[axis]; }

      const ssize_t& index (size_t axis) const { return pos[axis];  }
      ssize_t& index (size_t axis) { return pos[axis]; }

      friend std::ostream& operator<< (std::ostream& stream, const NeighbourhoodIterator& V) {
        stream << "iterator, position [ ";
        for (size_t n = 0; n < V.ndim(); ++n)
          stream << V.index(n) << " ";
        stream << "]";
        return stream;
      }


    private:
      std::vector<ssize_t> dim, offset, pos, ext;

      void value () const { assert (0); }
      // TODO how do I overwrite the index assignment operator?
      // void index (size_t axis) const { assert (0); }
  };

  //! @}
}

#endif



