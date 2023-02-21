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


#include "command.h"
#include "math/erfinv.h"

using namespace MR;
using namespace App;

//#define ERFINV_PRINT_ALL

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";
  SYNOPSIS = "Verify correct operation of the Math::erfinv() function";
  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}

void run ()
{
#ifndef ERFINV_PRINT_ALL
  constexpr default_type tolerance = 8.0 * std::numeric_limits<default_type>::epsilon();
#endif
  for (ssize_t i = -100; i <= 100; ++i) {
    const default_type f = 0.1 * i;
    const default_type p = std::erf (f);
    default_type q, z;
    if (f < 0) {
      const default_type erfc_negf = std::erfc (-f);
      q = 2.0 - erfc_negf;
      z = (p < -0.5) ? -Math::erfcinv (erfc_negf) : -Math::erfinv (-p);
    } else {
      q = std::erfc (f);
      z = (p > 0.5) ? Math::erfcinv (q) : Math::erfinv (p);
    }
#ifdef ERFINV_PRINT_ALL
    std::cerr << "  f = " << std::setw (15) << std::setprecision (15) << f
              << "; p = " << std::setw (15) << std::setprecision (15) << p
              << "; q = " << std::setw (15) << std::setprecision (15) << q
              << "; z = " << std::setw (15) << std::setprecision (15) << z << "\n";
#else
    if (std::abs (f - z) > tolerance) {
      Exception e ("erfinv() function tolerance above threshold:");
      e.push_back ("f = " + str(f) + "; p = " + str(p) + "; q = " + str(q) + "; z = " + str(z));
      e.push_back ("Error: " + str(f-z));
      e.push_back ("Tolerance: " + str(tolerance));
      throw e;
    }
#endif
  }
}

