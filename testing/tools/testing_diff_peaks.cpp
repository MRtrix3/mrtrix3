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

#include "command.h"
#include "datatype.h"
#include "fmt.h"
#include "progressbar.h"
#include <fmt/format.h>

#include "algo/threaded_loop.h"
#include "image.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compare two peak images for differences, within specified tolerance";

  ARGUMENTS
  + Argument ("peaks1", "a peaks image.").type_image_in()
  + Argument ("peaks2", "another peaks image.").type_image_in()
  + Argument ("tolerance", "the dot product difference to consider acceptable").type_float(0.0);

}
// clang-format on

void run() {
  auto in1 = Image<double>::open(argument[0]);
  auto in2 = Image<double>::open(argument[1]);
  check_dimensions(in1, in2);
  if (in1.ndim() != 4)
    throw Exception(fmt::format("images \"{}\" and \"{}\" are not 4D", in1.name(), in2.name()));
  if (in1.size(3) % 3)
    throw Exception(fmt::format("images \"{}\" and \"{}\" do not contain XYZ peak directions", in1.name(), in2.name()));
  if (!voxel_grids_match_in_scanner_space(in1, in2))
    throw Exception(fmt::format("images \"{}\" and \"{}\" do not reside on same voxel grid", in1.name(), in2.name()));

  double tol = argument[2];

  ThreadedLoop(in1, 0, 3).run(
      [&tol](decltype(in1) &a, decltype(in2) &b) {
        for (size_t i = 0; i != size_t(a.size(3)); i += 3) {
          Eigen::Vector3d veca, vecb;
          for (size_t axis = 0; axis != 3; ++axis) {
            a.index(3) = b.index(3) = i + axis;
            veca[axis] = a.value();
            vecb[axis] = b.value();
          }
          const double norma = veca.norm(), normb = vecb.norm();
          veca.normalize();
          vecb.normalize();
          const double dp = abs(veca.dot(vecb));
          if (norma && normb && (1.0 - dp > tol))
            throw Exception(fmt::format("images \"{}\" and \"{}\" do not match within specified precision of {}"
                                        " ( {} vs {}, norms [{} {}], dot product = {})",
                                        a.name(),
                                        b.name(),
                                        tol,
                                        veca.cast<float>(),
                                        vecb.cast<float>(),
                                        norma,
                                        normb,
                                        dp));
        }
      },
      in1,
      in2);

  CONSOLE("data checked OK");
}
