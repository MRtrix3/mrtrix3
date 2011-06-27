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

#ifndef __dataset_iterator_h__
#define __dataset_iterator_h__

#include <vector>

#include "types.h"

namespace MR
{
  namespace DataSet
  {

    /** \defgroup loop Looping functions
      @{ */

    //! a DataSet to iterate over, useful for multi-threaded looping.
    class Iterator
    {
      public:
        //! Constructor
        Iterator (ssize_t x, ssize_t y = 0, ssize_t z = 0, ssize_t dim3 = 0, ssize_t dim4 = 0, ssize_t dim5 = 0) 
        {
          if (x) {
            d.push_back (x);
            if (y) {
              d.push_back (y);
              if (z) {
                d.push_back (z);
                if (dim3) {
                  d.push_back (dim3);
                  if (dim4) {
                    d.push_back (dim4);
                    if (dim5) 
                      d.push_back (dim5);
                  }
                }
              }
            }
          }

          p.assign (ndim(), 0);
        }

        template <class Set> 
          Iterator (const Set& S) : 
            d (S.ndim()), 
            p (S.ndim(), 0) {
              for (size_t i = 0; i < S.ndim(); ++i)
                d[i] = S.dim(i);
            }

        size_t ndim () const { return d.size(); }
        ssize_t dim (size_t axis) const { return d[axis]; }

        const ssize_t& operator[] (size_t axis) const { return p[axis]; }
        ssize_t& operator[] (size_t axis) { return p[axis]; }

      private:
        std::vector<ssize_t> d, p;

        void value () const { assert (0); }
    };

    //! @}
  }
}

#endif



