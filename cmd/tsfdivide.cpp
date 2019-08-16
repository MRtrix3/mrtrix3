/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Divide corresponding values in track scalar files";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file.").type_file_in()
  + Argument ("input",  "the input track scalar file.").type_file_in()
  + Argument ("output", "the output track scalar file").type_file_out();
}

using value_type = float;


void run ()
{
  DWI::Tractography::Properties properties1;
  DWI::Tractography::ScalarReader<value_type> reader1 (argument[0], properties1);
  DWI::Tractography::Properties properties2;
  DWI::Tractography::ScalarReader<value_type> reader2 (argument[1], properties2);
  DWI::Tractography::ScalarWriter<value_type> writer (argument[2], properties1);

  DWI::Tractography::check_properties_match (properties1, properties2, "scalar", false);

  vector<value_type> tck_scalar1;
  vector<value_type> tck_scalar2;
  while (reader1 (tck_scalar1)) {
    if (!reader2 (tck_scalar2))
      break;
    if (tck_scalar1.size() != tck_scalar2.size())
      throw Exception ("track scalar length mismatch");

    vector<value_type> tck_scalar_output (tck_scalar1.size());
    for (size_t i = 0; i < tck_scalar1.size(); ++i) {
      if (tck_scalar2[i] == 0.0)
        tck_scalar_output[i] = 0;
      else
        tck_scalar_output[i] = tck_scalar1[i] / tck_scalar2[i];
    }
    writer (tck_scalar_output);
  }
}

