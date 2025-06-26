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

#pragma once

#include "denoise/estimator/base.h"
#include "denoise/estimator/result.h"

namespace MR::Denoise::Estimator {

template <ssize_t version> class Exp : public Base {
public:
  Exp() {}
  ~Exp() {}
  Result operator()(const Eigen::VectorBlock<eigenvalues_type> s,    //
                    const ssize_t m,                                 //
                    const ssize_t n,                                 //
                    const ssize_t rp,                                //
                    const Eigen::Vector3d & /*unused*/) const final; //
};

} // namespace MR::Denoise::Estimator
