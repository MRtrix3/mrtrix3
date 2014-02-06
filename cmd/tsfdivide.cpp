/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 31/01/2013

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "an application to divide corresponding values in track scalar files";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file.").type_file ()
  + Argument ("input",  "the input track scalar file.").type_file ()
  + Argument ("output", "the output track scalar file");
}

typedef float value_type;


void run ()
{
  DWI::Tractography::Properties properties1;
  DWI::Tractography::ScalarReader<value_type> reader1 (argument[0], properties1);
  DWI::Tractography::Properties properties2;
  DWI::Tractography::ScalarReader<value_type> reader2 (argument[1], properties2);
  DWI::Tractography::ScalarWriter<value_type> writer (argument[2], properties1);

  if (properties1.timestamp != properties2.timestamp)
    throw Exception ("the input scalar files do not relate to the same "
                     "tractography file (as defined by the timestamp in the header)");

  std::vector<value_type> tck_scalar1;
  std::vector<value_type> tck_scalar2;
  while (reader1 (tck_scalar1)) {
    reader2 (tck_scalar2);
    if (tck_scalar1.size() != tck_scalar2.size())
      throw Exception ("track scalar length mismatch");

    std::vector<value_type> tck_scalar_output (tck_scalar1.size());
    for (size_t i = 0; i < tck_scalar1.size(); ++i) {
      if (tck_scalar2[i] == 0.0)
        tck_scalar_output[i] = 0;
      else
        tck_scalar_output[i] = tck_scalar1[i] / tck_scalar2[i];
    }
    writer (tck_scalar_output);
  }
}

