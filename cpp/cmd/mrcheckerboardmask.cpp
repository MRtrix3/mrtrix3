/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include <cmath>

#include "algo/loop.h"
#include "command.h"
#include "image.h"
#include "image_helpers.h"

using namespace MR;
using namespace App;

constexpr ssize_t default_number_tiles = 5;

// clang-format off
void usage() {

  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Create bitwise checkerboard image";

  ARGUMENTS
  + Argument ("input", "the input image to be used as a template.").type_image_in ()
  + Argument ("output", "the output binary image mask.").type_image_out ();

  OPTIONS
  + Option ("tiles", "specify the number of tiles in any direction"
                     " (default: " + str(default_number_tiles) + ")")
    + Argument ("value").type_integer()

  + Option ("invert", "invert output binary mask.")

  + Option ("nan", "use NaN as the output zero value.");

}
// clang-format on

void run() {

  const size_t ntiles = get_option_value("tiles", default_number_tiles);
  const bool invert = !get_options("invert").empty();
  const bool use_NaN = !get_options("nan").empty();

  auto in = Image<float>::open(argument[0]);
  check_3D_nonunity(in);

  const size_t patchwidth_x = std::ceil((float)in.size(0) / (float)ntiles);
  const size_t patchwidth_y = std::ceil((float)in.size(1) / (float)ntiles);
  const size_t patchwidth_z = std::ceil((float)in.size(2) / (float)ntiles);

  Header header_out(in);
  header_out.datatype() = use_NaN ? DataType::Float32 : DataType::Bit;
  auto out = Image<float>::create(argument[1], header_out);

  float zero = use_NaN ? NAN : 0.0;
  float one = 1.0;
  if (invert)
    std::swap(zero, one);

  for (auto l = Loop(in)(in, out); l; ++l) {
    const size_t x = in.index(0);
    const size_t y = in.index(1);
    const size_t z = in.index(2);
    size_t xpatch = (x - (x % patchwidth_x)) / patchwidth_x;
    size_t ypatch = (y - (y % patchwidth_y)) / patchwidth_y;
    size_t zpatch = (z - (z % patchwidth_z)) / patchwidth_z;
    out.value() = ((xpatch % 2 + ypatch % 2 + zpatch % 2) % 2 == 0) ? one : zero;
  }
}
