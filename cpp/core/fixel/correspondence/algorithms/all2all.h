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
#include "fixel/correspondence/fixel.h"

namespace MR::Fixel::Correspondence::Algorithms {

#ifdef FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
// For the sake of testing, construct a correspondence algorithm with predictable behaviour:
//   assign all source fixels to every target fixel
class All2All : public Base {
public:
  All2All() {}
  virtual ~All2All() {}
  std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                      const std::vector<Correspondence::Fixel> &s,
                                                      const std::vector<Correspondence::Fixel> &t) const final;
};
#endif

} // namespace MR::Fixel::Correspondence::Algorithms
