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

#include "command.h"

#include "fixel/fixel.h"
#include "fixel/validate.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate the contents of a fixel directory";

  DESCRIPTION
  + "This command checks that a fixel directory conforms to the fixel directory format."
    " Specifically, it verifies:"
    " (1) a valid index image is present;"
    " (2) a valid directions image is present;"
    " (3) every fixel index is covered by exactly one voxel in the index image;"
    " (4) all fixel data files in the directory contain the same number of fixels"
    " as implied by the index image."
  + Fixel::format_description;

  ARGUMENTS
  + Argument ("fixel_directory", "the fixel directory to be validated").type_directory_in();
}
// clang-format on

void run() {
  Fixel::validate_directory(argument[0]);
  CONSOLE("fixel directory \"" + std::string(argument[0]) + "\" is valid");
}
