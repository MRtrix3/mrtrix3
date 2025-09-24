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

#include "dwi/tractography/resampling/resampling.h"

namespace MR::DWI::Tractography::Resampling {

class FixedNumPoints : public BaseCRTP<FixedNumPoints> {

public:
  FixedNumPoints() : num_points(0) {}

  FixedNumPoints(const size_t n) : num_points(n) {}

  bool operator()(const Streamline<> &, Streamline<> &) const override;
  bool valid() const override { return num_points; }

  void set_num_points(const size_t n) { num_points = n; }
  size_t get_num_points() const { return num_points; }

private:
  size_t num_points;
};

} // namespace MR::DWI::Tractography::Resampling
