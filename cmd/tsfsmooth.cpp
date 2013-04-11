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

#include "app.h"
#include "math/median.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Gaussian filter a track scalar file";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file.").type_file ()
  + Argument ("output", "the output track scalar file");
}

typedef float value_type;


void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::ScalarReader<value_type> reader (argument[0], properties);
  DWI::Tractography::ScalarWriter<value_type> writer (argument[1], properties);

  std::vector<value_type> tck_scalar;
  std::vector<value_type> values;
  while (reader.next(tck_scalar)) {
    std::vector<value_type> tck_scalar_mean (tck_scalar.size());

    for (size_t i = 0; i < tck_scalar.size(); ++i) {
      values.clear();
      if (i > 2)
        values.push_back(tck_scalar[i-3]);
      if (i > 1)
        values.push_back(tck_scalar[i-2]);
      if (i > 0)
        values.push_back(tck_scalar[i-1]);
      if (i < tck_scalar.size() - 1 )
        values.push_back(tck_scalar[i+1]);
      if (i < tck_scalar.size() - 2 )
        values.push_back(tck_scalar[i+2]);
      if (i < tck_scalar.size() - 3 )
        values.push_back(tck_scalar[i+3]);
      values.push_back(tck_scalar[i]);
      float mean_val = 0.0;
      for (int j = 0; j < 5; j++)
        mean_val += values[j];
      mean_val /= 5.0;
      tck_scalar_mean[i] = mean_val;
    }
    writer.append (tck_scalar_mean);
  }
}

