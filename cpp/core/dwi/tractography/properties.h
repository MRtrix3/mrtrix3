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

#include "app.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/list.h"
#include "timer.h"
#include "types.h"
#include <map>

namespace MR::DWI::Tractography {

constexpr ssize_t file_timestamp_precision = 20;

void check_timestamps(const Properties &, const Properties &, std::string_view);
void check_counts(const Properties &, const Properties &, std::string_view, bool abort_on_fail);

class Properties : public KeyValues {
public:
  Properties() { set_timestamp(); }

  void set_timestamp();
  void set_version_info();
  void update_command_history();
  void clear();

  template <typename T> void set(T &variable, std::string_view name) {
    const std::string key(name);
    if ((*this)[key].empty())
      (*this)[key] = str(variable);
    else
      variable = to<T>((*this)[key]);
  }

  float get_stepsize() const;
  void compare_stepsize_rois() const;

  // In use at time of execution
  ROIUnorderedSet include, exclude, mask;
  ROIOrderedSet ordered_include;
  Seeding::List seeds;

  // As stored within the header of an existing .tck file
  std::multimap<std::string, std::string> prior_rois;

  std::vector<std::string> comments;

  friend std::ostream &operator<<(std::ostream &stream, const Properties &P);
};

} // namespace MR::DWI::Tractography
