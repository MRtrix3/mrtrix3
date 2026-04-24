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

  SYNOPSIS = "Validate a track scalar file against its corresponding tractogram";

  DESCRIPTION
  + "This command checks that a track scalar file (.tsf) is consistent with"
    " the tractogram (.tck) from which it was derived."
    " The following conditions are verified:"

  + "1. Both files must contain a \"timestamp\" field in their headers,"
    " and the two timestamps must be identical."
    " The timestamp is copied from the tractogram into the scalar file"
    " when the scalar file is created,"
    " so a mismatch indicates that the scalar file was not produced from the supplied tractogram."

  + "2. The track scalar file header must contain a \"count\" field."

  + "3. The value of the \"count\" field in the track scalar file header"
    " must match the number of scalar sequences actually present in the file."

  + "4. The number of scalar sequences in the track scalar file"
    " must equal the number of streamlines in the tractogram."

  + "5. For every streamline, the number of scalar values in the corresponding scalar sequence"
    " must equal the number of vertices in the streamline.";

  ARGUMENTS
  + Argument ("tsf",    "the input track scalar file").type_file_in()
  + Argument ("tracks", "the tractogram from which the track scalar file was derived").type_tracks_in();
}
// clang-format on

void run() {
  // validate_tsf() throws an Exception with a descriptive message on any failure.
  validate_tsf(argument[0], argument[1]);
  CONSOLE("Track scalar file \"" + std::string(argument[0]) + "\"" +                    //
          " is valid with respect to tractogram \"" + std::string(argument[1]) + "\""); //
}
