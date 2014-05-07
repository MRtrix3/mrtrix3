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
#include "math/median.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"


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

  OPTIONS
  + Option ("stdev", "apply Gaussian smoothing with the specified standard deviation. "
            "The standard deviation is defined in units of track points (Default: 4)")
  + Argument ("sigma").type_float();
}

typedef float value_type;


void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::ScalarReader<value_type> reader (argument[0], properties);
  DWI::Tractography::ScalarWriter<value_type> writer (argument[1], properties);

  Options opt = get_options("stdev");
  float stdev = 4.0;
  if (opt.size())
    stdev = opt[0][0];

  std::vector<float> kernel (2 * ceil(2.5 * stdev) + 1, 0);
  float norm_factor = 0.0;
  float radius = (kernel.size() - 1.0) / 2.0;
  for (size_t c = 0; c < kernel.size(); ++c) {
    kernel[c] = exp(-(c - radius) * (c - radius)  / (2 * stdev * stdev));
    norm_factor += kernel[c];
  }
  for (size_t c = 0; c < kernel.size(); c++)
    kernel[c] /= norm_factor;

  std::vector<value_type> tck_scalar;
  while (reader (tck_scalar)) {
    std::vector<value_type> tck_scalars_smoothed (tck_scalar.size());

    for (size_t i = 0; i < tck_scalar.size(); ++i) {
      float norm_factor = 0.0;
      float value = 0.0;
      for (int k = -(int)radius; k <= (int)radius; ++k) {
        if (i + k >= 0 && i + k < tck_scalar.size()) {
          value += kernel[k + radius] * tck_scalar[i + k];
          norm_factor += kernel[k + radius];
        }
      }
      tck_scalars_smoothed[i] = value / norm_factor;
    }
    writer (tck_scalars_smoothed);
  }
}

