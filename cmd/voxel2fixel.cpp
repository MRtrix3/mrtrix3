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

  for (auto i = Loop ("mapping voxel scalar values to fixels", 0, 3)(scalar, fixel_template, output); i; ++i) {
    output.value().set_size (fixel_template.value().size());
    for (size_t f = 0; f != fixel_template.value().size(); ++f) {
      output.value()[f] = fixel_template.value()[f];
      output.value()[f].value = scalar.value();
    }
  }
}
