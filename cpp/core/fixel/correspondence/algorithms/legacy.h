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

#include "types.h"

#include "fixel/correspondence/algorithms/base.h"

namespace MR::App {
class OptionGroup;
}

namespace MR::Fixel::Correspondence::Algorithms {

extern App::OptionGroup LegacyOptions;

// Replicate the unweighted nearest-fixel behaviour of the fixelcorrespondence
//   command from MRtrix versions 3.0.x and earlier:
// For each target fixel, select the nearest source fixel with weight 1.0,
//   provided the angle between them is within the threshold.
// Unlike algorithm "nearest", no normalisation is applied when multiple target
//   fixels select the same source fixel.
class Legacy : public Base {
public:
  Legacy(const float max_angle) : dp_threshold(std::cos(max_angle * Math::pi / 180.0)) {}
  virtual ~Legacy() {}

  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t) const final;

protected:
  const float dp_threshold;
};

} // namespace MR::Fixel::Correspondence::Algorithms
