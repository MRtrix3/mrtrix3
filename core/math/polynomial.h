/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "math/math.h"

namespace MR
{
  namespace Math
  {


    //! Evaluate a polynomial expansion for a scalar term
    template <class Cont>
    default_type polynomial (Cont& coeffs, const default_type x)
    {
      if (!coeffs.size())
        return NaN;
      default_type result = coeffs[coeffs.size()-1];
      for (ssize_t i = coeffs.size() - 2; i >= 0; --i) {
        result *= x;
        result += default_type(coeffs[i]);
      }
      return result;
    }



  }
}

