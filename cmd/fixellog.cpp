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
  + "compute the natural logarithm of all values in a fixel image";

  ARGUMENTS
  + Argument ("input", "the input fixel image.").type_image_in ()
  + Argument ("output", "the output fixel image.").type_image_out ();
}


void run ()
{
  auto header = Header::open (argument[0]);
  Sparse::Image<FixelMetric> input (argument[0]);

  Sparse::Image<FixelMetric> output (argument[1], header);

  for (auto i = Loop ("computing log", input) (input, output); i; ++i) {
    output.value().set_size (input.value().size());
    for (size_t fixel = 0; fixel != input.value().size(); ++fixel) {
      output.value()[fixel] = input.value()[fixel];
      output.value()[fixel].value = std::log (input.value()[fixel].value);
    }
  }
}

