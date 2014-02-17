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

