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

#include "app.h"
#include "header.h"

namespace MR::App {
class OptionGroup;
} // namespace MR::App

namespace MR::DWI::Tractography {
class Properties;
} // namespace MR::DWI::Tractography

namespace MR::DWI::Tractography::ACT {

// If the sum of tissue probabilities is below this threshold, the image is being exited, so a boolean flag is thrown
// The values will however still be accessible
constexpr default_type tissuesum_threshold = 0.5;

// Actually think it's preferable to not use these
constexpr default_type wm_pathintegral_threshold = 0.0;
constexpr default_type wm_maxabs_threshold = 0.0;

// Absolute value of tissue proportion difference
constexpr default_type gmwmi_accuracy = 0.01;

// Number of times a backtrack attempt will be made from a certain maximal track length
//   before the length of truncation is increased
constexpr ssize_t backtrack_attempts = 3;

extern const App::OptionGroup ACTOption;

void load_act_properties(Properties &properties);

void verify_5TT_image(const Header &);

} // namespace MR::DWI::Tractography::ACT
