/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

  const Eigen::Array<ssize_t, 3, 1> patchwidths{
      static_cast<ssize_t>(std::ceil(static_cast<default_type>(in.size(0)) / static_cast<default_type>(ntiles))),
      static_cast<ssize_t>(std::ceil(static_cast<default_type>(in.size(1)) / static_cast<default_type>(ntiles))),
      static_cast<ssize_t>(std::ceil(static_cast<default_type>(in.size(2)) / static_cast<default_type>(ntiles)))};

  Header header_out(in);
  header_out.datatype() = use_NaN ? DataType::Float32 : DataType::Bit;
  auto out = Image<float>::create(argument[1], header_out);

  float zero = use_NaN ? NaNF : 0.0;
  float one = 1.0;
  if (invert)
    std::swap(zero, one);

  for (auto l = Loop(in)(in, out); l; ++l) {
    const Eigen::Array<ssize_t, 3, 1> pos{in.index(0), in.index(1), in.index(2)};
    const Eigen::Array<ssize_t, 3, 1> patch{(pos[0] - (pos[0] % patchwidths[0])) / patchwidths[0],
                                            (pos[1] - (pos[1] % patchwidths[1])) / patchwidths[1],
                                            (pos[2] - (pos[2] % patchwidths[2])) / patchwidths[2]};
    out.value() = ((patch[0] % 2 + patch[1] % 2 + patch[2] % 2) % 2 == 0) ? one : zero;
  }
}
