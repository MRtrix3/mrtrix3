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
  + "an application to threshold and invert track scalar files";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file.").type_file ()
  + Argument ("N",      "the desired threshold").type_float ()
  + Argument ("output", "the binary output track scalar file");


  OPTIONS
  + Option ("invert", "invert the output mask");

}

typedef float value_type;


void run ()
{
  bool invert = get_options("invert").size() ? true : false;
  float threshold = argument[1];

  DWI::Tractography::Properties properties;
  DWI::Tractography::ScalarReader<value_type> reader (argument[0], properties);
  DWI::Tractography::ScalarWriter<value_type> writer (argument[2], properties);

  std::vector<value_type> tck_scalar;
  while (reader (tck_scalar)) {
    std::vector<value_type> tck_mask (tck_scalar.size());
    for (size_t i = 0; i < tck_scalar.size(); ++i) {
      if (invert) {
        if (tck_scalar[i] > threshold)
          tck_mask[i] = 0.0;
        else
          tck_mask[i] = 1.0;
      } else {
        if (tck_scalar[i] > threshold)
          tck_mask[i] = 1.0;
        else
          tck_mask[i] = 0.0;
      }
    }
    writer (tck_mask);
  }
}

