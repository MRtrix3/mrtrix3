/*

 Implementation based on the GSL (http://www.gnu.org/software/gsl/)

*/

#ifndef __math_chebyshev_h__
#define __math_chebyshev_h__

#include "math/math.h"

namespace MR
{
  namespace Math
  {
    namespace Chebyshev
    {

      template <typename T> inline T eval (const double* coef, const int order, const T lower, const T upper, const T x)
      {
        T y  = (2.0*x - lower - upper) / (upper - lower);
        T d  = 0.0, dd = 0.0;
        for (int i = order; i >= 1; i--) {
          T temp = d;
          d = 2.0*y*d - dd + coef[i];
          dd = temp;
        }
        return (y*d - dd + 0.5 * coef[0]);
      }

    }
  }
}

#endif

