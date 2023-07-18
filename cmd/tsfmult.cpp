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

  SYNOPSIS = "Multiply corresponding values in track scalar files";

  ARGUMENTS
  + Argument ("input1", "the first input track scalar file.").type_file_in()
  + Argument ("input1", "the second input track scalar file.").type_file_in()
  + Argument ("output", "the output track scalar file").type_file_out();
}

using value_type = float;


void run ()
{
  DWI::Tractography::Properties properties1, properties2;
  DWI::Tractography::ScalarReader<value_type> reader1 (argument[0], properties1);
  DWI::Tractography::ScalarReader<value_type> reader2 (argument[1], properties2);
  DWI::Tractography::check_properties_match (properties1, properties2, "scalar", false);

  DWI::Tractography::ScalarWriter<value_type> writer (argument[2], properties1);
  DWI::Tractography::TrackScalar<> tck_scalar1, tck_scalar2, tck_scalar_output;
  while (reader1 (tck_scalar1)) {
    if (!reader2 (tck_scalar2)) {
      WARN ("No more track scalars left in input file \"" + std::string(argument[1]) +
            "\" after " + str(tck_scalar1.get_index()+1) + " streamlines; " +
            "but more data are present in input file \"" + std::string(argument[0]) + "\"");
      break;
    }
    if (tck_scalar1.size() != tck_scalar2.size())
      throw Exception ("track scalar length mismatch at streamline index " + str(tck_scalar1.get_index()));

    tck_scalar_output.set_index (tck_scalar1.get_index());
    tck_scalar_output.resize (tck_scalar1.size());
    for (size_t i = 0; i < tck_scalar1.size(); ++i) {
      tck_scalar_output[i] = tck_scalar1[i] * tck_scalar2[i];
    }
    writer (tck_scalar_output);
  }
  if (reader2 (tck_scalar2)) {
    WARN ("No more track scalars left in input file \"" + std::string(argument[0]) +
          "\" after " + str(tck_scalar1.get_index()+1) + " streamlines; " +
          "but more data are present in input file \"" + std::string(argument[1]) + "\"");
  }
}

