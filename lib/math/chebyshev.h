/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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



/* 
 
 Implementation based on the GSL (http://www.gnu.org/software/gsl/)

*/

#ifndef __math_chebyshev_h__
#define __math_chebyshev_h__

#include "math/math.h"

namespace MR {
  namespace Math {
    namespace Chebyshev {

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

