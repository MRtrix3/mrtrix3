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
#include "mrtrix.h"

#include "dwi/directions/validate.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Directions;

// clang-format off
void usage() {

  AUTHOR = "Robert Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a direction set file";

  DESCRIPTION
  + "This command checks that a direction set text file is well-formed."
    " The format is inferred from the number of columns:"

  + "2 columns: spherical coordinates (azimuth, inclination)."
    " The azimuth must lie strictly within (-2π, 2π)."
    " The inclination must lie within [-π, π]."

  + "3 columns: Cartesian unit directions (x, y, z)."
    " All three components must lie within [-1, 1]."
    " The Euclidean norm of each direction must not exceed 1.0."
    " The command reports to the console if any directions are not of unit norm,"
    " but this is not treated as a hard error."

  + "4 columns: diffusion gradient table (x, y, z, b-value)."
    " All direction components must lie within [-1, 1]."
    " The b-value must be non-negative."
    " For non-zero b-value entries, the gradient direction must be of unit norm."
    " b=0 entries may carry a zero direction vector.";

  ARGUMENTS
  + Argument ("dirs", "the input direction set text file").type_file_in();
}
// clang-format on

void run() {
  const DirectionsValidation result = validate_directions(argument[0]);

  const std::string fmt = result.format == DirectionsFormat::Spherical   ? "spherical coordinates"
                          : result.format == DirectionsFormat::Cartesian ? "Cartesian unit directions"
                                                                         : "diffusion gradient table";

  CONSOLE("Direction file \"" + std::string(argument[0]) +
          "\":"
          " " +
          str(result.n_directions) +
          " direction(s)"
          " in " +
          fmt + " format");

  if (result.format == DirectionsFormat::Cartesian) {
    if (result.n_non_unit)
      CONSOLE(str(result.n_non_unit) + " of " + str(result.n_directions) + " direction(s) are not of unit norm");
    else
      CONSOLE("All directions are of unit norm");
  }
}
