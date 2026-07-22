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

#include "connectome/connectome.h"

namespace MR::Connectome {

using namespace App;
// clang-format off
const OptionGroup MatrixOutputOptions =
    OptionGroup("Options for outputting connectome matrices")
    + Option("symmetric", "Make matrices symmetric on output")
    + Option("zero_self",
             "Set self-connections to zero on output"
             " (the matrix diagonal, or for a connectivity vector the single self-connection)")
      .alias("zero_diagonal");
// clang-format on
} // namespace MR::Connectome
