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

#include "dwi/directions/file.h"

#include "cmdline_option.h"
#include "math/math.h"

namespace MR::DWI::Directions {

App::Option cartesian_option = App::Option("cartesian",
                                           "Output directions in Cartesian coordinates [x y z]"
                                           " instead of spherical angles [az in].");

Eigen::MatrixXd load_spherical(std::string_view filename) {
  auto directions = File::Matrix::load_matrix<>(filename);
  if (directions.cols() == 2)
    return directions;
  if (directions.cols() != 3)
    throw Exception("unexpected number of columns for directions file \"" + filename + "\"");

  return Math::Sphere::cartesian2spherical(directions);
}

Eigen::MatrixXd load_cartesian(std::string_view filename) {
  auto directions = File::Matrix::load_matrix<>(filename);
  if (directions.cols() == 2)
    directions = Math::Sphere::spherical2cartesian(directions);
  else {
    if (directions.cols() != 3)
      throw Exception("unexpected number of columns for directions file \"" + filename + "\"");
    for (ssize_t n = 0; n < directions.rows(); ++n) {
      auto norm = directions.row(n).norm();
      if (std::fabs(default_type(1.0) - norm) > 1.0e-4)
        WARN("directions file \"" + filename + "\" contains non-unit direction vectors");
      directions.row(n).array() *= norm ? default_type(1.0) / norm : default_type(0.0);
    }
  }
  return directions;
}

} // namespace MR::DWI::Directions
