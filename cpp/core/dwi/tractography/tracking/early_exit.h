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

#include <Eigen/Core>

#if EIGEN_VERSION_AT_LEAST(3, 3, 0)
#define TCKGEN_EARLY_EXIT_USE_FULL_BINOMIAL
#include <unsupported/Eigen/SpecialFunctions>
#endif

#include "dwi/tractography/tracking/shared.h"

namespace MR::DWI::Tractography::Tracking {

class EarlyExit {
public:
  static const default_type probability_threshold;
  EarlyExit(const SharedBase &shared)
      : max_num_seeds(shared.max_num_seeds),
        max_num_tracks(shared.max_num_tracks),
        counter(0),
        next_test((max_num_seeds && max_num_tracks) ? (10 * max_num_seeds / max_num_tracks) : 0) {}

  bool operator()(const size_t, const size_t);

private:
  const size_t max_num_seeds, max_num_tracks;
  size_t counter, next_test;

  static const default_type zvalue_threshold;
  static const default_type cease_testing_percentage;
};

} // namespace MR::DWI::Tractography::Tracking
