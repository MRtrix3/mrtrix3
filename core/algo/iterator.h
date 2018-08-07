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


#ifndef __algo_iterator_h__
#define __algo_iterator_h__

#include "types.h"

namespace MR
{

  /** \defgroup loop Looping functions
    @{ */

  //! a dummy image to iterate over, useful for multi-threaded looping.
  class Iterator { NOMEMALIGN
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
      vector<ssize_t> d, p;

      void value () const { assert (0); }
  };

  //! @}
}

#endif



