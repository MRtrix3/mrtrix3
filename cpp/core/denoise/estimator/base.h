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

#include "denoise/denoise.h"
#include "denoise/estimator/result.h"

namespace MR::Denoise::Estimator {

class Base {
public:
  Base() = default;
  Base(const Base &) = delete;
  virtual void update_vst_image(Image<float> &) {}
  // m = Number of image volumes;
  // n = Number of voxels in patch;
  // rp = Preconditioner rank = number of means regressed from the data;
  // pos = realspace position of the centre of the patch
  virtual Result operator()(const Eigen::VectorBlock<eigenvalues_type> eigenvalues, //
                            const ssize_t m,                                        //
                            const ssize_t n,                                        //
                            const ssize_t rp,                                       //
                            const Eigen::Vector3d &pos) const = 0;                  //
};

} // namespace MR::Denoise::Estimator
