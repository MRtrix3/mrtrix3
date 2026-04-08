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

#pragma once

#include <limits>

#include "types.h"

namespace MR::Math {

class MeanAndVariance {
public:
  MeanAndVariance() : mean(default_type(0)), variance(default_type(0)) {}
  default_type std() const { return std::sqrt(variance); }
  default_type mean;
  default_type variance;
};

template <class Cont> MeanAndVariance welford(const Cont &data) {
  MeanAndVariance result;
  default_type m2(default_type(0));
  size_t count(size_t(0));
  default_type delta(std::numeric_limits<default_type>::quiet_NaN());
  default_type delta2(std::numeric_limits<default_type>::quiet_NaN());
  for (size_t i = 0; i != data.size(); ++i) {
    ++count;
    delta = data[i] - result.mean;
    result.mean += delta / count;
    delta2 = data[i] - result.mean;
    m2 += delta * delta2;
  }
  result.variance = count > 1 ? m2 / (count - 1) : std::numeric_limits<default_type>::quiet_NaN();
  return result;
}

} // namespace MR::Math
