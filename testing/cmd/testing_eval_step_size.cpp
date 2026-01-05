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

#include <limits>

#include "command.h"
#include "types.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

#define DEFAULT_HAUSDORFF 1e-5
#define DEFAULT_MAXFAIL 0

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compute the step sizes within a track file";

  ARGUMENTS
  + Argument ("tck", "the input track file").type_tracks_in ();
}



void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<> reader (argument[0], properties);
  DWI::Tractography::Streamline<> tck;

  while (reader (tck)) {
    if (tck.size() < 2) {
      std::cout << "NaN\n";
    } else {
      DWI::Tractography::Streamline<>::point_type p (tck[0]);
      for (size_t i = 1; i != tck.size(); ++i) {
        std::cout << (tck[i]-p).norm() << " ";
        p = tck[i];
      }
      std::cout << "\n";
    }
  }

}
