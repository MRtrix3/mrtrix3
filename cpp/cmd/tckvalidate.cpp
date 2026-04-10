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

#include "dwi/tractography/validate.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a tractogram (.tck) file";

  DESCRIPTION
  + "This command checks that a tractogram file is well-formed."
    " The binary data section of a .tck file consists of a sequence of"
    " 3-float triplets."
    " Each triplet must be exactly one of:"
    " a regular vertex (all three components finite),"
    " an inter-streamline delimiter (all three components NaN),"
    " or the mandatory end-of-file barrier (all three components infinity)."
    " The end-of-file barrier must be the last triplet in the file."

  + "The following hard violations cause the command to fail:"
    " (1) a triplet that is partially non-finite"
    " (i.e. not all-finite, not all-NaN, and not all-infinity);"
    " (2) any data present after the end-of-file barrier;"
    " (3) the end of the binary data section is reached without an end-of-file barrier"
    " (truncated file);"
    " (4) the last streamline body is not terminated by a NaN delimiter"
    " before the end-of-file barrier;"
    " (5) the \"count\" field is absent from the file header;"
    " (6) the \"count\" field in the file header does not match"
    " the number of streamlines actually present in the file."

  + "The command also reports the presence of streamlines"
    " with zero vertices or exactly one vertex,"
    " which are degenerate cases that may indicate issues with the"
    " tractography algorithm that produced the file.";

  ARGUMENTS
  + Argument ("tracks_in", "the input tractogram file").type_tracks_in();
}
// clang-format on

void run() {
  // validate_tck() throws on any hard format or metadata violation.
  const TckValidation result = validate_tck(argument[0]);

  CONSOLE("Tractogram \"" + std::string(argument[0]) + "\" is valid: " + //
          str(result.n_streamlines) + " streamline(s)");                 //

  if (result.n_empty > 0) {
    WARN(str(result.n_empty) + " empty streamline(s) (0 vertices) found");
  }

  if (result.n_single_vertex > 0) {
    WARN(str(result.n_single_vertex) + " single-vertex streamline(s) found");
  }

  if (result.n_empty == 0 && result.n_single_vertex == 0) {
    CONSOLE("All streamlines have two or more vertices");
  }
}
