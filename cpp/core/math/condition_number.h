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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/SVD>
#pragma GCC diagnostic pop

#include "types.h"

namespace MR::Math {

template <class M> default_type condition_number(const M &data) {
  assert(data.rows() && data.cols());
  auto v = Eigen::JacobiSVD<M>(data).singularValues();
  return v[0] / v[v.size() - 1];
}

// Explicit instantiation in core/math/condition_number.cpp
extern template default_type
condition_number<Eigen::Matrix<default_type, -1, -1>>(const Eigen::Matrix<default_type, -1, -1> &data);

} // namespace MR::Math
