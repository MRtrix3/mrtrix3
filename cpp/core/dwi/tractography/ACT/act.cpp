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

#include "dwi/tractography/ACT/act.h"

#include <filesystem>
#include <string>

#include "app.h"
#include "dwi/tractography/properties.h"
#include "enum.h"

namespace MR::DWI::Tractography::ACT {

using namespace App;

// clang-format off
const OptionGroup ACTOption =
    OptionGroup("Anatomically-Constrained Tractography options")

    + Option("act",
             "use the Anatomically-Constrained Tractography framework during tracking; "
             "provided image must be in the 5TT (five-tissue-type) format")
      + Argument("image").type_image_in()

    + Option("backtrack",
             "allow tracks to be truncated and re-tracked if a poor structural termination is encountered")

    + Option("crop_at_gmwmi",
             "crop streamline endpoints more precisely as they cross the GM-WM interface")

    + Option("sgm_truncation",
             fmt::format("control how truncation of streamlines is performed"
                         " if they attempt to enter and then exit sub-cortical grey matter;"
                         " options are: {}", Enum::join<sgm_trunc_t>()))
      + Argument ("choice").type_choice<sgm_trunc_t>();
// clang-format on

void load_act_properties(Properties &properties) {
  auto opt = App::get_options("act");
  if (!opt.empty()) {

    properties["act"] = opt[0][0].as_text();
    opt = get_options("backtrack");
    if (!opt.empty())
      properties["backtrack"] = "1";
    opt = get_options("crop_at_gmwmi");
    if (!opt.empty())
      properties["crop_at_gmwmi"] = "1";
    opt = get_options("sgm_truncation");
    if (!opt.empty())
      properties["sgm_truncation"] = opt[0][0].as_text();

  } else {

    if (!get_options("backtrack").empty())
      WARN("ignoring -backtrack option: only valid if using ACT");
    if (!get_options("crop_at_gmwmi").empty())
      WARN("ignoring -crop_at_gmwmi option: only valid if using ACT");
    if (!get_options("sgm_truncation").empty())
      WARN("ignoring -sgm_truncation option: only valid if using ACT");
  }
}

} // namespace MR::DWI::Tractography::ACT
