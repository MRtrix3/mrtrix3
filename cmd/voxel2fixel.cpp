/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 2015

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
#include "progressbar.h"
#include "algo/loop.h"
#include "image.h"
#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"

using namespace MR;
using namespace App;

using Sparse::FixelMetric;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "map the scalar value in each voxel to all fixels within that voxel. "
    "This could be used to enable CFE-based statistical analysis to be performed on voxel-wise measures";

  ARGUMENTS
  + Argument ("image_in",  "the input image.").type_image_in ()
  + Argument ("fixel_in",  "the input fixel image.").type_image_in ()
  + Argument ("fixel_out", "the output fixel image.").type_image_out ();
}


void run ()
{
  auto scalar = Image<float>::open (argument[0]);
  auto fixel_header = Header::open (argument[1]);
  check_dimensions (scalar, fixel_header, 0, 3);
  Sparse::Image<FixelMetric> fixel_template (argument[1]);
  Sparse::Image<FixelMetric> output (argument[2], fixel_header);

  for (auto i = Loop ("mapping voxel scalar values to fixels ...", 0, 3)(scalar, fixel_template, output); i; ++i) {
    output.value().set_size (fixel_template.value().size());
    for (size_t f = 0; f != fixel_template.value().size(); ++f) {
      output.value()[f] = fixel_template.value()[f];
      output.value()[f].value = scalar.value();
    }
  }
}
