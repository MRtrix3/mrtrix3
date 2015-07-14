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

#ifndef __algo_iterator_h__
#define __algo_iterator_h__

#include <vector>

#include "types.h"

namespace MR
{

  /** \defgroup loop Looping functions
    @{ */

  //! a dummy image to iterate over, useful for multi-threaded looping.
  class Iterator
  {
    public:
      Iterator() = delete;
      template <class InfoType>
        Iterator (const InfoType& S) :
          d (S.ndim()),
          p (S.ndim(), 0) {
            for (size_t i = 0; i < S.ndim(); ++i)
              d[i] = S.size(i);
          }

      size_t ndim () const { return d.size(); }
      ssize_t size (size_t axis) const { return d[axis]; }

      const ssize_t& index (size_t axis) const { return p[axis]; }
      ssize_t& index (size_t axis) { return p[axis]; }

      friend std::ostream& operator<< (std::ostream& stream, const Iterator& V) {
        stream << "iterator, position [ ";
        for (size_t n = 0; n < V.ndim(); ++n)
          stream << V.index(n) << " ";
        stream << "]";
        return stream;
      }

    private:
      std::vector<ssize_t> d, p;

      void value () const { assert (0); }
  };

  //! @}
}

#endif



