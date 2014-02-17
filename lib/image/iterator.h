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



