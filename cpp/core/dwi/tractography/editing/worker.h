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

#include <string>

#include "types.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Editing {

class Worker {

public:
  Worker(Tractography::Properties &p, const bool inv, const bool end)
      : properties(p),
        inverse(inv),
        ends_only(end),
        thresholds(p),
        include_visitation(properties.include, properties.ordered_include) {}

  Worker(const Worker &that)
      : properties(that.properties),
        inverse(that.inverse),
        ends_only(that.ends_only),
        thresholds(that.thresholds),
        include_visitation(properties.include, properties.ordered_include) {}

  bool operator()(Streamline<> &, Streamline<> &) const;

private:
  const Tractography::Properties &properties;
  const bool inverse, ends_only;

  class Thresholds {
  public:
    Thresholds(Tractography::Properties &);
    Thresholds(const Thresholds &);
    bool operator()(const Streamline<> &) const;

  private:
    float max_length, min_length;
    float max_weight, min_weight;
    float step_size;
  } thresholds;

  mutable IncludeROIVisitation include_visitation;
};

} // namespace MR::DWI::Tractography::Editing
