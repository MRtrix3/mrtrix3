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

#pragma once

#include "math/math.h"

namespace MR::Math {

template <class VarArrayType, class CountArrayType>
default_type welch_satterthwaite(const VarArrayType &variances, const CountArrayType &counts) {
  assert(static_cast<size_t>(variances.size()) == static_cast<size_t>(counts.size()));
  default_type numerator = 0.0, denominator = 0.0;
  for (size_t i = 0; i != static_cast<size_t>(variances.size()); ++i) {
    const default_type ks2 = (1.0 / (counts[i] - 1)) * variances[i];
    numerator += ks2;
    denominator += Math::pow2(ks2) / (counts[i] - 1);
  }
  return Math::pow2(numerator) / denominator;
}

} // namespace MR::Math
