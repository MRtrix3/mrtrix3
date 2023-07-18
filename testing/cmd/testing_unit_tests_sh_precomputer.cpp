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

#include <sstream>

#include "command.h"
#include "math/SH.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Test the accuracy of the spherical harmonic precomputer";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}

using value_type = float;
using coefs_type = Eigen::Matrix<value_type,Eigen::Dynamic,1>;
using dir_type = Eigen::Matrix<value_type,3,1>;



void run ()
{
  using namespace Math::SH;

  const int lmax = 8;

  coefs_type coefs = coefs_type::Random (NforL (lmax));
  PrecomputedAL<value_type> precomputer (lmax);

  for (size_t n = 0; n < 10000; ++n) {
    dir_type direction = dir_type::Random().normalized();
    if (std::abs (value (coefs, direction, lmax) - precomputer.value (coefs, direction)) > 1e-3)
      throw Exception ("difference exceeds tolerance");
  }

}

