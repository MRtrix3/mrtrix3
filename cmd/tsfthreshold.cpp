/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Threshold and invert track scalar files";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file.").type_file_in()
  + Argument ("T",      "the desired threshold").type_float ()
  + Argument ("output", "the binary output track scalar file").type_file_out();


  OPTIONS
  + Option ("invert", "invert the output mask");

}

using value_type = float;


void run ()
{
  bool invert = get_options("invert").size() ? true : false;
  float threshold = argument[1];

  DWI::Tractography::Properties properties;
  DWI::Tractography::ScalarReader<value_type> reader (argument[0], properties);
  DWI::Tractography::ScalarWriter<value_type> writer (argument[2], properties);

  DWI::Tractography::TrackScalar<value_type> tck_scalar;
  while (reader (tck_scalar)) {
    DWI::Tractography::TrackScalar<value_type> tck_mask (tck_scalar.size());
    tck_mask.set_index (tck_scalar.get_index());
    for (size_t i = 0; i < tck_scalar.size(); ++i) {
      if (invert) {
        if (tck_scalar[i] > threshold)
          tck_mask[i] = value_type(0);
        else
          tck_mask[i] = value_type(1);
      } else {
        if (tck_scalar[i] > threshold)
          tck_mask[i] = value_type(1);
        else
          tck_mask[i] = value_type(0);
      }
    }
    writer (tck_mask);
  }
}

