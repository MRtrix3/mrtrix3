/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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

