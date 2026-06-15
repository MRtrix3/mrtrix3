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

#include "dwi/tractography/editing/editing.h"

#include "exception.h"

namespace MR::DWI::Tractography::Editing {

using namespace App;

// clang-format off
const OptionGroup LengthOption =
    OptionGroup("Streamline length threshold options")
    + Option("maxlength",
             "set the maximum length of any streamline in mm")
      + Argument("value").type_float(0.0)
    + Option("minlength",
             "set the minimum length of any streamline in mm")
      + Argument("value").type_float(0.0);

const OptionGroup TruncateOption =
    OptionGroup("Streamline count truncation options")
    + Option("number",
             "set the desired number of selected streamlines to be propagated to the output file")
      + Argument("count").type_integer(1)
    + Option("skip",
             "omit this number of selected streamlines before commencing writing to the output file")
      + Argument("count").type_integer(1);

const OptionGroup WeightsOption =
    OptionGroup("Thresholds pertaining to per-streamline weighting")
    + Option("maxweight",
             "set the maximum weight of any streamline")
      + Argument("value").type_float(0.0)
    + Option("minweight",
             "set the minimum weight of any streamline")
      + Argument("value").type_float(0.0);

const OptionGroup FieldFilterOption =
    OptionGroup("Options for thresholding based on arbitrary streamline data fields")
    + Option("dps_min",
             "retain only those streamlines for which the named per-streamline field"
             " is greater than or equal to the specified value").allow_multiple()
      + Argument("field").type_text()
      + Argument("value").type_float()
    + Option("dps_max",
             "retain only those streamlines for which the named per-streamline field"
             " is less than or equal to the specified value").allow_multiple()
      + Argument("field").type_text()
      + Argument("value").type_float()
    + Option("dpv_min",
             "retain only those streamline vertices for which the named per-vertex field"
             " is greater than or equal to the specified value").allow_multiple()
      + Argument("field").type_text()
      + Argument("value").type_float()
    + Option("dpv_max",
             "retain only those streamline vertices for which the named per-vertex field"
             " is less than or equal to the specified value").allow_multiple()
      + Argument("field").type_text()
      + Argument("value").type_float();
// clang-format on

void load_properties(Tractography::Properties &properties) {

  // LengthOption
  auto opt = get_options("maxlength");
  if (!opt.empty()) {
    if (properties.find("max_dist") == properties.end()) {
      properties["max_dist"] = static_cast<std::string>(opt[0][0]);
    } else {
      try {
        const float maxlength = std::min(static_cast<float>(opt[0][0]), to<float>(properties["max_dist"]));
        properties["max_dist"] = str(maxlength);
      } catch (Exception &) {
        DEBUG("Corrupted pre-existing property field \"max_dist\"; applying user request as-is");
        properties["max_dist"] = static_cast<std::string>(opt[0][0]);
      }
    }
  }
  opt = get_options("minlength");
  if (!opt.empty()) {
    if (properties.find("min_dist") == properties.end()) {
      properties["min_dist"] = static_cast<std::string>(opt[0][0]);
    } else {
      try {
        const float minlength = std::max(static_cast<float>(opt[0][0]), to<float>(properties["min_dist"]));
        properties["min_dist"] = str(minlength);
      } catch (Exception &) {
        DEBUG("Corrupted pre-existing property field \"min_dist\"; applying user request as-is");
        properties["min_dist"] = static_cast<std::string>(opt[0][0]);
      }
    }
  }

  // TruncateOption
  // These have no influence on Properties

  // WeightsOption
  // Only the thresholds have an influence on Properties
  opt = get_options("maxweight");
  if (!opt.empty())
    properties["max_weight"] = static_cast<std::string>(opt[0][0]);
  opt = get_options("minweight");
  if (!opt.empty())
    properties["min_weight"] = static_cast<std::string>(opt[0][0]);
}

FieldFilters load_field_filters(const FieldRegistry &registry) {
  FieldFilters result;

  // Resolve every instance of one option name into \a destination, looking the
  //   named field up in the input registry under \a role.
  const auto resolve = [&registry](const std::string_view option_name,
                                   const FieldRole role,
                                   const Bound bound,
                                   std::vector<FieldFilter> &destination) {
    const std::string option = std::string("-").append(option_name);
    const std::string role_text = (role == FieldRole::DPV) ? "per-vertex" : "per-streamline";
    for (const auto &instance : get_options(option_name)) {
      const std::string field_name = instance[0].as_text();
      const float value = static_cast<float>(instance[1]);
      const FieldDescriptor *descriptor = registry.find(field_name, role);
      if (descriptor == nullptr)
        throw Exception("input tractogram carries no " + role_text + " field named \"" + field_name +
                        "\" to threshold with " + option);
      if (descriptor->columns != 1)
        throw Exception(role_text + " field \"" + field_name + "\" has " + str(descriptor->columns) +
                        " columns; only a single-column (scalar) field can be thresholded with " + option);
      destination.push_back({descriptor->ordinal, bound, value, field_name});
    }
  };

  resolve("dps_min", FieldRole::DPS, Bound::Min, result.dps);
  resolve("dps_max", FieldRole::DPS, Bound::Max, result.dps);
  resolve("dpv_min", FieldRole::DPV, Bound::Min, result.dpv);
  resolve("dpv_max", FieldRole::DPV, Bound::Max, result.dpv);

  return result;
}

} // namespace MR::DWI::Tractography::Editing
