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

#include <magic_enum/magic_enum.hpp>

namespace MR::DWI::Tractography::Mapping {

enum class contrast_t { TDI, LENGTH, INVLENGTH, SCALAR_MAP, SCALAR_MAP_COUNT, FOD_AMP, CURVATURE, VECTOR_FILE };
struct Strings {
  std::string choice;
  std::string description;
};
extern const std::unordered_map<contrast_t, Strings> contrast_names;

enum class vox_stat_t { SUM, MIN, MEAN, MAX };
extern const std::unordered_map<vox_stat_t, std::string> voxel_statistic_names;

// Note: ENDS_CORR is meaningful internally (TW-dFC) but is excluded from magic_enum
//   reflection below, so it is not offered as a command-line option.
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

} // namespace MR::DWI::Tractography::Mapping

// Exclude ENDS_CORR from magic_enum reflection of tck_stat_t: it is used internally
//   (TW-dFC) but is not a valid "-stat_tck" command-line choice. This makes
//   type_choice<tck_stat_t>() omit it from the presented choices and
//   MR::Enum::from_name<tck_stat_t>("ends_corr") reject it. The hand-written
//   track_statistic_names map (twi_stats.cpp) retains ENDS_CORR for internal
//   stringification, independently of magic_enum.
template <>
constexpr magic_enum::customize::customize_t
magic_enum::customize::enum_name<MR::DWI::Tractography::Mapping::tck_stat_t>(
    MR::DWI::Tractography::Mapping::tck_stat_t value) noexcept {
  return value == MR::DWI::Tractography::Mapping::tck_stat_t::ENDS_CORR //
             ? magic_enum::customize::invalid_tag
             : magic_enum::customize::default_tag;
}
