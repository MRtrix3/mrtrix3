/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "an application to multiply corresponding values in track scalar files";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file.").type_file_in()
  + Argument ("input",  "the input track scalar file.").type_file_in()
  + Argument ("output", "the output track scalar file").type_file_out();
}

typedef float value_type;


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
      tck_scalar_output[i] = tck_scalar1[i] * tck_scalar2[i];
    }
    writer (tck_scalar_output);
  }
}

