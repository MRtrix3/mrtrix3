/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __image_iterator_h__
#define __image_iterator_h__

#include <vector>

#include "types.h"

namespace MR
{
  namespace Image
  {

    /** \defgroup loop Looping functions
      @{ */

    //! a dummy image to iterate over, useful for multi-threaded looping.
    class Iterator
    {
      public:
        template <class InfoType>
          Iterator (const InfoType& S) :
            d (S.ndim()),
            p (S.ndim(), 0) {
              for (size_t i = 0; i < S.ndim(); ++i)
                d[i] = S.dim(i);
            }

        size_t ndim () const { return d.size(); }
        ssize_t dim (size_t axis) const { return d[axis]; }

        const ssize_t& operator[] (size_t axis) const { return p[axis]; }
        ssize_t& operator[] (size_t axis) { return p[axis]; }

        friend std::ostream& operator<< (std::ostream& stream, const Iterator& V) {
          stream << "iterator, position [ ";
          for (size_t n = 0; n < V.ndim(); ++n)
            stream << V[n] << " ";
          stream << "]";
          return stream;
        }

      private:
        std::vector<ssize_t> d, p;

        void value () const { assert (0); }
    };

    //! @}
  }
}

#endif



