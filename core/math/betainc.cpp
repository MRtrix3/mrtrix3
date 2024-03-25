/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <math.h>

#include "math/betainc.h"

#include "mrtrix.h"

namespace MR::Math {

/*
 * zlib License
 *
 * Regularized Incomplete Beta Function
 *
 * Copyright (c) 2016, 2017 Lewis Van Winkle
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

// The original code from the source above has been modified in the
//   following (inconsequential) ways:
// - Changed function and constants names
// - Check that inputs a and b are both positive
// - Use lgamma_r() rather than std::lgamma() for thread-safety if available;
//   if not available, lock across threads just for execution of std::lgamma()
// - Changed double to default_type
// - Changed formatting to match MRtrix3 convention

#define BETAINCREG_STOP 1.0e-8
#define BETAINCREG_TINY 1.0e-30

#ifdef MRTRIX_HAVE_LGAMMA_R
extern "C" double lgamma_r(double, int *);
#endif

default_type betaincreg(const default_type a, const default_type b, const default_type x) {
  if (a <= 0.0 || b <= 0.0 || x < 0.0 || x > 1.0)
    throw Exception("Invalid inputs: MR::Math::betaincreg(" + str(a) + ", " + str(b) + ", " + str(x) + ")");

  // The continued fraction converges nicely for x < (a+1)/(a+b+2)
  if (x > (a + 1.0) / (a + b + 2.0)) {
    return (1.0 - betaincreg(b, a, 1.0 - x)); // Use the fact that beta is symmetrical
  }

  // Find the first part before the continued fraction
#ifdef MRTRIX_HAVE_LGAMMA_R
  int dummy_sign = 0;
  const default_type lbeta_ab = lgamma_r(a, &dummy_sign) + lgamma_r(b, &dummy_sign) - lgamma_r(a + b, &dummy_sign);
#else
  default_type lbeta_ab = 0.0;
  {
    std::lock_guard<std::mutex> lock(mutex_lgamma);
    lbeta_ab = std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b);
  }
#endif
  const default_type front = std::exp(std::log(x) * a + std::log(1.0 - x) * b - lbeta_ab) / a;

  // Use Lentz's algorithm to evaluate the continued fraction
  default_type f = 1.0, c = 1.0, d = 0.0;

  for (size_t i = 0; i <= 200; ++i) {
    const size_t m = i / 2;

    default_type numerator;
    if (!i) {
      numerator = 1.0; // First numerator is 1.0
    } else if (i % 2) {
      numerator = -((a + m) * (a + b + m) * x) / ((a + 2.0 * m) * (a + 2.0 * m + 1)); // Odd term
    } else {
      numerator = (m * (b - m) * x) / ((a + 2.0 * m - 1.0) * (a + 2.0 * m)); // Even term
    }

    // Do an iteration of Lentz's algorithm
    d = 1.0 + numerator * d;
    if (abs(d) < BETAINCREG_TINY)
      d = BETAINCREG_TINY;
    d = 1.0 / d;

    c = 1.0 + numerator / c;
    if (abs(c) < BETAINCREG_TINY)
      c = BETAINCREG_TINY;

    const default_type cd = c * d;
    f *= cd;

    // Check for stop
    if (abs(1.0 - cd) < BETAINCREG_STOP)
      return front * (f - 1.0);
  }

  return NaN; // Needed more loops, did not converge
}

#ifndef MRTRIX_HAVE_LGAMMA_R
std::mutex mutex_lgamma;
#endif

} // namespace MR::Math
