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

#include <string>
#include <unordered_map>
#include <vector>

namespace MR::DWI::Tractography::Mapping {

enum class contrast_t { TDI, LENGTH, INVLENGTH, SCALAR_MAP, SCALAR_MAP_COUNT, FOD_AMP, CURVATURE, VECTOR_FILE };
struct Strings {
  std::string choice;
  std::string description;
};
extern const std::unordered_map<contrast_t, Strings> contrast_names;
extern const std::vector<std::string> contrasts;

enum class vox_stat_t { SUM, MIN, MEAN, MAX };
extern const std::unordered_map<vox_stat_t, std::string> voxel_statistic_names;
extern const std::vector<std::string> voxel_statistics;

// Note: ENDS_CORR not provided as a command-line option
enum class tck_stat_t {
  SUM,
  MIN,
  MEAN,
  MAX,
  MEDIAN,
  MEAN_NONZERO,
  GAUSSIAN,
  ENDS_MIN,
  ENDS_MEAN,
  ENDS_MAX,
  ENDS_PROD,
  ENDS_CORR
};
extern const std::unordered_map<tck_stat_t, Strings> track_statistic_names;
extern const std::vector<std::string> track_statistics;

} // namespace MR::DWI::Tractography::Mapping
