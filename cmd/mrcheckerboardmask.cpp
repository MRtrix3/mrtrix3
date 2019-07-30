/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <cmath>

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Create bitwise checkerboard image";

  ARGUMENTS
  + Argument ("input", "the input image to be used as a template.").type_image_in ()
  + Argument ("output", "the output binary image mask.").type_image_out ();

  OPTIONS
  + Option ("tiles", "specify the number of tiles in any direction")
  + Argument ("value").type_integer()

  + Option ("invert", "invert output binary mask.")

  + Option ("nan", "use NaN as the output zero value.");
}


void run ()
{

  size_t ntiles = 5;
  auto opt = get_options ("tiles");
  if (opt.size()) {
    ntiles = opt[0][0];
  }

  bool invert = get_options ("invert").size();
  const bool use_NaN = get_options ("nan").size();

  auto in = Image<float>::open (argument[0]);
  check_3D_nonunity (in);

  size_t patchwidth_x = ceil((float) in.size(0) / ntiles);
  size_t patchwidth_y = ceil((float) in.size(1) / ntiles);
  size_t patchwidth_z = ceil((float) in.size(2) / ntiles);

  Header header_out (in);
  header_out.datatype() = use_NaN ? DataType::Float32 : DataType::Bit;
  auto out = Image<float>::create (argument[1], header_out);

  float zero = use_NaN ? NAN : 0.0;
  float one  = 1.0;
  if (invert) std::swap (zero, one);

  for (auto l = Loop(in) (in, out); l; ++l) {
    const size_t x = in.index(0);
    const size_t y = in.index(1);
    const size_t z = in.index(2);
    size_t xpatch = (x-(x%patchwidth_x))/patchwidth_x;
    size_t ypatch = (y-(y%patchwidth_y))/patchwidth_y;
    size_t zpatch = (z-(z%patchwidth_z))/patchwidth_z;
    out.value() = (
      (xpatch%2 + ypatch%2+ zpatch%2)%2==0)  ? one : zero;
  }

}
