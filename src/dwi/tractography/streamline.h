#ifndef __dwi_tractography_streamline_h__
#define __dwi_tractography_streamline_h__


#include <vector>

#include "point.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      template <typename T = float>
        class Streamline : public std::vector< Point<T> >
      {
        public:
          typedef T value_type;
          Streamline () : index (-1), weight (value_type (1.0)) { }

          Streamline (size_t size) : 
            std::vector< Point<value_type> > (size), 
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (const std::vector< Point<value_type> >& tck) :
            std::vector< Point<value_type> > (tck),
            index (-1),
            weight (1.0) { }

          void clear()
          {
            std::vector< Point<T> >::clear();
            index = -1;
            weight = 1.0;
          }

          size_t index;
          value_type weight;
      };



    }
  }
}


#endif

