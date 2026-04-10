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

#include <Eigen/Dense>
#include <cmath>
#include <string>

#include "app.h"
#include "dwi/gradient.h"
#include "exception.h"
#include "file/matrix.h"
#include "math/math.h"
#include "mrtrix.h"

namespace MR::DWI::Directions {

template <class MatrixType>
const DirectionsValidation validate(const MatrixType &M, std::string_view path, const bool permit_gradtable) {
  using value_type = typename MatrixType::Scalar;
  if (!M.rows())
    throw Exception("Direction set is empty");

  const ssize_t ncols = M.cols();
  DirectionsValidation result;
  result.n_directions = static_cast<size_t>(M.rows());

  constexpr value_type two_pi = 2.0 * Math::pi;
  constexpr value_type unit_tol = 1e-4;
  constexpr value_type range_tol = 1e-5 * Math::pi;

  try {

    switch (ncols) {

    case 2: {
      // ---------------------------------------------------------------
      // 2-column spherical coordinates: (azimuth, inclination).
      // Azimuth must lie strictly within [-2π, 2π].
      // Azimuth range must not exceed 2π.
      // Inclination must lie within [-π, π].
      // Inclination range must not exceed π.
      // ---------------------------------------------------------------
      result.format = DirectionsFormat::Spherical;
      value_type az_min = two_pi;
      value_type az_max = -two_pi;
      value_type inc_min = Math::pi;
      value_type inc_max = -Math::pi;
      for (ssize_t r = 0; r < M.rows(); ++r) {
        const value_type az = M(r, 0);
        const value_type inc = M(r, 1);
        az_min = std::min(az_min, az);
        az_max = std::max(az_max, az);
        inc_min = std::min(inc_min, inc);
        inc_max = std::max(inc_max, inc);
        if (az < -two_pi || az > two_pi)
          throw Exception("Row " + str(r + 1) + ": " + //
                          "azimuth value " + str(az) + " is outside the permitted range [-2π, 2π]");
        if (inc < -Math::pi || inc > Math::pi)
          throw Exception("Row " + str(r + 1) + ": " + //
                          "inclination value " + str(inc) + " is outside the permitted range [-π, π]");
      }
      if (az_max - az_min > two_pi - range_tol)
        throw Exception("Range of azimuth values exceeds 2π");
      if (inc_max - inc_min > Math::pi - range_tol)
        throw Exception("Range of inclination values exceeds π");

    } break;

    case 3: {
      // ---------------------------------------------------------------
      // 3-column Cartesian directions: (x, y, z).
      // All components must lie within [-1, 1].
      // The Euclidean norm must not exceed 1.0.
      // Directions that are not of unit norm are counted but not rejected.
      // ---------------------------------------------------------------
      result.format = DirectionsFormat::Cartesian;
      for (ssize_t r = 0; r < M.rows(); ++r) {
        const value_type x = M(r, 0);
        const value_type y = M(r, 1);
        const value_type z = M(r, 2);
        if (x < -1.0 || x > 1.0)
          throw Exception("Row " + str(r + 1) + ": " +                                          //
                          "x component " + str(x) + " is outside the permitted range [-1, 1]"); //
        if (y < -1.0 || y > 1.0)
          throw Exception("Row " + str(r + 1) + ": " +                                          //
                          "y component " + str(y) + " is outside the permitted range [-1, 1]"); //
        if (z < -1.0 || z > 1.0)
          throw Exception("Row " + str(r + 1) + ": " +                                          //
                          "z component " + str(z) + " is outside the permitted range [-1, 1]"); //
        const double norm = std::sqrt(x * x + y * y + z * z);
        if (norm > 1.0 + unit_tol)
          throw Exception("Row " + str(r + 1) + ": " +                  //
                          "vector norm " + str(norm) + " exceeds 1.0"); //
        if (std::abs(norm - value_type(1)) > unit_tol)
          ++result.n_non_unit;
      }

    } break;

    case 4: {

      // TODO Technically this could be relaxed to be permissive of single-shell, removing b=0 volumes
      if (!permit_gradtable)
        throw Exception("Command can only handle direction sets on unit sphere, not gradient tables");

      // ---------------------------------------------------------------
      // 4-column diffusion gradient table: (x, y, z, b).
      // All direction components must lie within [-1, 1].
      // The Euclidean norm must not exceed 1.0.
      // The b-value must be non-negative.
      // For non-zero b-value entries the direction should be of unit norm;
      // directions that are not of unit norm are counted but not rejected.
      // b=0 entries (b ≤ bzero_threshold) may carry a zero direction vector.
      // ---------------------------------------------------------------
      result.format = DirectionsFormat::GradientTable;
      const value_type bthresh = DWI::bzero_threshold();
      for (ssize_t r = 0; r < M.rows(); ++r) {
        const value_type x = M(r, 0);
        const value_type y = M(r, 1);
        const value_type z = M(r, 2);
        const value_type b = M(r, 3);
        if (b < 0.0)
          throw Exception("Row " + str(r + 1) + ": " +           //
                          "b-value " + str(b) + " is negative"); //
        if (x < -1.0 || x > 1.0)
          throw Exception("Row " + str(r + 1) + ": " +                                          //
                          "x component " + str(x) + " is outside the permitted range [-1, 1]"); //
        if (y < -1.0 || y > 1.0)
          throw Exception("Row " + str(r + 1) + ": " +                                          //
                          "y component " + str(y) + " is outside the permitted range [-1, 1]"); //
        if (z < -1.0 || z > 1.0)
          throw Exception("Row " + str(r + 1) + ": " +                                          //
                          "z component " + str(z) + " is outside the permitted range [-1, 1]"); //
        if (b > bthresh) {
          // const value_type norm = M.block<1,3>(r, 0).norm();
          const value_type norm = M.block(r, 0, 1, 3).norm();
          if (norm > 1.0 + unit_tol)
            throw Exception("Row " + str(r + 1) + ": " +                  //
                            "vector norm " + str(norm) + " exceeds 1.0"); //
          if (std::abs(norm - value_type(1)) > unit_tol)
            ++result.n_non_unit;
        }
      }

    } break;

    default:
      throw Exception("Unexpected number of columns (" + str(ncols) + "): " +          //
                      "expected 2 (spherical), 3 (Cartesian), or 4 (gradient table)"); //
    }

  } catch (Exception &e) {
    throw Exception(e, "Direction file \"" + std::string(path) + "\" validation failed");
  }

  const std::string fmt = result.format == DirectionsFormat::Spherical   ? "spherical"
                          : result.format == DirectionsFormat::Cartesian ? "Cartesian"
                                                                         : "gradient table";
  DEBUG("Direction file \"" + std::string(path) + "\": " +                 //
        str(result.n_directions) + " direction(s) in " + fmt + " format"); //
  if (result.n_non_unit) {
    const std::string msg = "Direction file \"" + std::string(path) + "\": " +             //
                            str(result.n_non_unit) + " direction(s) are not of unit norm"; //
    if (result.format == DirectionsFormat::Cartesian) {
      WARN(msg);
    } else {
      // Well-formed mechanism to deal with non-unit-norm directions in a gradient table
      DEBUG(msg);
    }
  }

  return result;
}
template const DirectionsValidation
validate<Eigen::MatrixXf>(const Eigen::MatrixXf &M, std::string_view path, const bool permit_gradtable);
template const DirectionsValidation
validate<Eigen::MatrixXd>(const Eigen::MatrixXd &M, std::string_view path, const bool permit_gradtable);

} // namespace MR::DWI::Directions
