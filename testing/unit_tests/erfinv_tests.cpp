/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "gtest/gtest.h"

#include "math/erfinv.h"
#include "types.h"

#include <cmath>
#include <iomanip>
#include <limits>

constexpr MR::default_type global_tolerance = 8.0 * std::numeric_limits<MR::default_type>::epsilon();

TEST(ErfinvTest, InverseFunctionAccuracy) {

  for (ssize_t i = -100; i <= 100; ++i) {
    const MR::default_type f = static_cast<MR::default_type>(i) * 0.1;
    const MR::default_type p = std::erf(f);

    MR::default_type z = 0.0;
    MR::default_type q = 0.0;

    if (f < 0.0) {
      const MR::default_type erfc_negf = std::erfc(-f);
      q = 2.0 - erfc_negf;

      if (p < -0.5) {
        z = -MR::Math::erfcinv(erfc_negf);
      } else {
        z = -MR::Math::erfinv(-p);
      }
    } else {
      const MR::default_type erfc_f = std::erfc(f);
      q = erfc_f;

      if (p > 0.5) {
        z = MR::Math::erfcinv(erfc_f);
      } else {
        z = MR::Math::erfinv(p);
      }
    }

    EXPECT_NEAR(f, z, global_tolerance) << std::setprecision(std::numeric_limits<MR::default_type>::max_digits10)
                                        << "Test for i = " << i << ":\n"
                                        << "  f (input to erf)     = " << f << "\n"
                                        << "  p (erf(f))           = " << p << "\n"
                                        << "  q (debug value)      = " << q << "\n"
                                        << "  z (inverted f)       = " << z << "\n"
                                        << "  Difference (f-z)     = " << (f - z) << "\n"
                                        << "  Tolerance            = " << global_tolerance;
  }
}

TEST(ErfinvTest, EdgeCases) {
  const MR::default_type edge_tolerance = std::numeric_limits<MR::default_type>::epsilon();

  EXPECT_NEAR(static_cast<MR::default_type>(0.0), MR::Math::erfinv(static_cast<MR::default_type>(0.0)), edge_tolerance);
  EXPECT_NEAR(
      static_cast<MR::default_type>(0.0), MR::Math::erfcinv(static_cast<MR::default_type>(1.0)), edge_tolerance);
}
