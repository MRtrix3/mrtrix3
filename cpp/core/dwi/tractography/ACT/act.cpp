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
             "control how truncation of streamlines is performed"
             " if they attempt to enter and then exit sub-cortical grey matter.")
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
  }
  // When ACT is not in use, -backtrack / -crop_at_gmwmi / -sgm_truncation are never read; the
  //   unused-option check reports any of them that the user nonetheless specified.
}

} // namespace MR::DWI::Tractography::ACT
