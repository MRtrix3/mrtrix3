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

#ifndef __math_factorial_h__
#define __math_factorial_h__

#include <functional>
#include <limits>

namespace MR
{
  namespace Math
  {


    template <typename T>
    T factorial (const T i) {
      if (i < 2) {
        return T(1);
      } else if (i == std::numeric_limits<T>::max()) {
        return i;
      } else {
        const T multiplier = factorial<T>(i-1);
        const T result = i * multiplier;
        if (result / multiplier == i)
          return result;
        else
          return std::numeric_limits<T>::max();
      }
    };


  }
}

#endif
