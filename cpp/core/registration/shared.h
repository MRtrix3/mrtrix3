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

namespace MR::Registration {
using namespace MR;
using namespace App;
// clang-format off
const OptionGroup multiContrastOptions =
    OptionGroup("Multi-contrast options")
    + Option("mc_weights",
             "relative weight of images used for multi-contrast registration."
             " Default: 1.0 (equal weighting)")
      + Argument("weights").type_sequence_float();

const OptionGroup fod_options =
    OptionGroup("FOD registration options")
    + Option("directions",
             "provide a file containing the directions used for FOD reorienation using apodised point spread functions"
             " (Default: built-in 60-direction set)")
      + Argument("file").type_file_in()
    + Option("noreorientation",
             "turn off FOD reorientation."
             " Reorientation is on by default if the number of volumes in the 4th dimension"
             " corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series"
             " (i.e. 6, 15, 28, 45, 66 etc.)");
// clang-format on
} // namespace MR::Registration
