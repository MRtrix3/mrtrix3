/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "math/SH.h"
#include "gtest/gtest.h"

#include <Eigen/Core>
#include <cstddef>

using namespace MR;

using value_type = float;
using coefs_type = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;
using dir_type = Eigen::Matrix<value_type, 3, 1>;

class SphericalHarmonicPrecomputerTest : public ::testing::Test {};

TEST_F(SphericalHarmonicPrecomputerTest, Accuracy) {
  using namespace Math::SH;

  const int lmax = 8;
  const float tolerance = 1e-3F;

  const coefs_type coefs = coefs_type::Random(static_cast<Eigen::Index>(NforL(lmax)));
  const PrecomputedAL<value_type> precomputer(lmax);

  for (size_t n = 0; n < 10000; ++n) {
    dir_type direction = dir_type::Random().normalized();
    const value_type val_standard = value(coefs, direction, lmax);
    const value_type val_precomputed = precomputer.value(coefs, direction);
    ASSERT_NEAR(val_standard, val_precomputed, tolerance)
        << "Difference exceeds tolerance at iteration " << n << " for direction (" << direction.x() << ", "
        << direction.y() << ", " << direction.z() << ")";
  }
}
