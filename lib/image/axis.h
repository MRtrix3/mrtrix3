#ifndef __image_axis_h__
#define __image_axis_h__

#include "types.h"
#include "mrtrix.h"

namespace MR
{
  namespace Image
  {

    class Axis
    {
      public:
        Axis () : dim (1), vox (NAN), stride (0) { }

        int     dim;
        float   vox;
        ssize_t stride;

        bool  forward () const {
          return (stride > 0);
        }
        ssize_t direction () const {
          return (stride > 0 ? 1 : -1);
        }

        friend std::ostream& operator<< (std::ostream& stream, const Axis& axes);

        static std::vector<ssize_t> parse (size_t ndim, const std::string& specifier);
        static void check (const std::vector<ssize_t>& parsed, size_t ndim);
    };

    //! @}

  }
}

#endif


