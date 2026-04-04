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

#include "dwi/directions/validate.h"

#include <cmath>
#include <string>

#include "app.h"
#include "dwi/gradient.h"
#include "exception.h"
#include "file/matrix.h"
#include "math/math.h"
#include "mrtrix.h"

namespace MR::DWI::Directions {

DirectionsValidation validate_directions(std::string_view path) {
  const auto M = File::Matrix::load_matrix<double>(path);
  if (!M.rows())
    throw Exception("Direction file \"" + std::string(path) + "\": no data found");

  const ssize_t ncols = M.cols();
  DirectionsValidation result;
  result.n_directions = static_cast<size_t>(M.rows());

  constexpr double two_pi = 2.0 * Math::pi;
  constexpr double unit_tol = 1e-4;

  if (ncols == 2) {
    // ---------------------------------------------------------------
    // 2-column spherical coordinates: (azimuth, inclination).
    // Azimuth must lie strictly within (-2π, 2π).
    // Inclination must lie within [-π, π].
    // ---------------------------------------------------------------
    result.format = DirectionsFormat::Spherical;
    for (ssize_t r = 0; r < M.rows(); ++r) {
      const double az = M(r, 0);
      const double inc = M(r, 1);
      if (az <= -two_pi || az >= two_pi)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": azimuth value " + str(az) + " is outside the permitted range (-2π, 2π)");
      if (inc < -Math::pi || inc > Math::pi)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": inclination value " + str(inc) + " is outside the permitted range [-π, π]");
    }

  } else if (ncols == 3) {
    // ---------------------------------------------------------------
    // 3-column Cartesian directions: (x, y, z).
    // All components must lie within [-1, 1].
    // The Euclidean norm must not exceed 1.0.
    // Directions that are not of unit norm are counted but not rejected.
    // ---------------------------------------------------------------
    result.format = DirectionsFormat::Cartesian;
    for (ssize_t r = 0; r < M.rows(); ++r) {
      const double x = M(r, 0), y = M(r, 1), z = M(r, 2);
      if (x < -1.0 || x > 1.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": x component " + str(x) + " is outside the permitted range [-1, 1]");
      if (y < -1.0 || y > 1.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": y component " + str(y) + " is outside the permitted range [-1, 1]");
      if (z < -1.0 || z > 1.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": z component " + str(z) + " is outside the permitted range [-1, 1]");
      const double norm = std::sqrt(x * x + y * y + z * z);
      if (norm > 1.0 + unit_tol)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": vector norm " + str(norm) + " exceeds 1.0");
      if (std::abs(norm - 1.0) > unit_tol)
        ++result.n_non_unit;
    }

  } else if (ncols == 4) {
    // ---------------------------------------------------------------
    // 4-column diffusion gradient table: (x, y, z, b).
    // All direction components must lie within [-1, 1].
    // The b-value must be non-negative.
    // For non-zero b-value entries the direction must be of unit norm.
    // b=0 entries (b ≤ bzero_threshold) may carry a zero direction vector.
    // ---------------------------------------------------------------
    result.format = DirectionsFormat::GradientTable;
    const double bthresh = DWI::bzero_threshold();
    for (ssize_t r = 0; r < M.rows(); ++r) {
      const double x = M(r, 0), y = M(r, 1), z = M(r, 2), b = M(r, 3);
      if (b < 0.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": b-value " + str(b) + " is negative");
      if (x < -1.0 || x > 1.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": x component " + str(x) + " is outside the permitted range [-1, 1]");
      if (y < -1.0 || y > 1.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": y component " + str(y) + " is outside the permitted range [-1, 1]");
      if (z < -1.0 || z > 1.0)
        throw Exception("Direction file \"" + std::string(path) +
                        "\":"
                        " row " +
                        str(r + 1) + ": z component " + str(z) + " is outside the permitted range [-1, 1]");
      if (b > bthresh) {
        const double norm = std::sqrt(x * x + y * y + z * z);
        if (std::abs(norm - 1.0) > unit_tol)
          throw Exception("Direction file \"" + std::string(path) +
                          "\":"
                          " row " +
                          str(r + 1) + ": gradient direction for b=" + str(b) + " has non-unit norm " + str(norm));
      }
    }

  } else {
    throw Exception("Direction file \"" + std::string(path) +
                    "\":"
                    " unexpected number of columns (" +
                    str(ncols) + "); expected 2 (spherical), 3 (Cartesian), or 4 (gradient table)");
  }

  return result;
}

void debug_validate_directions(std::string_view path) {
  if (App::log_level < 3)
    return;
  try {
    const DirectionsValidation v = validate_directions(path);
    const std::string fmt = v.format == DirectionsFormat::Spherical   ? "spherical"
                            : v.format == DirectionsFormat::Cartesian ? "Cartesian"
                                                                      : "gradient table";
    DEBUG("Direction file \"" + std::string(path) + "\": " + str(v.n_directions) + " direction(s) in " + fmt +
          " format");
    if (v.n_non_unit)
      DEBUG("Direction file \"" + std::string(path) + "\": " + str(v.n_non_unit) +
            " direction(s) are not of unit norm");
  } catch (const Exception &e) {
    DEBUG("Direction file \"" + std::string(path) + "\": validation failed: " + e[0]);
  }
}

} // namespace MR::DWI::Directions
