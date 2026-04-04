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

#pragma once

#include <cstddef>
#include <string_view>

namespace MR::DWI::Directions {

//! The format inferred from the column count of the direction file.
enum class DirectionsFormat {
  Spherical,    //!< 2-column: (azimuth, inclination)
  Cartesian,    //!< 3-column: (x, y, z) unit directions
  GradientTable //!< 4-column: (x, y, z, b-value)
};

//! Summary of a full direction-file validation.
struct DirectionsValidation {

  //! The format inferred from the number of columns.
  DirectionsFormat format = DirectionsFormat::Spherical;

  //! Number of directions (rows) in the file.
  size_t n_directions = 0;

  //! Number of directions whose Euclidean norm differs from 1.0 by more than
  //! the validation tolerance.  Only meaningful for the Cartesian format.
  size_t n_non_unit = 0;
};

//! Validate a direction set text file and return a summary of findings.
//!
//! The format is inferred from the number of columns:
//!
//!   2 columns — spherical coordinates (azimuth, inclination):
//!     - azimuth must lie strictly within (-2π, 2π)
//!     - inclination must lie within [-π, π]
//!
//!   3 columns — Cartesian unit directions (x, y, z):
//!     - all components must lie within [-1, 1]
//!     - the Euclidean norm of each direction must not exceed 1.0
//!     - directions that are not of unit norm are counted in n_non_unit
//!       (this is reported but not treated as a hard error)
//!
//!   4 columns — diffusion gradient table (x, y, z, b):
//!     - all direction components must lie within [-1, 1]
//!     - the b-value must be non-negative
//!     - for non-zero b-value rows the direction must be of unit norm
//!
//! Any other column count is a hard error.
//! Throws Exception on the first hard violation.
DirectionsValidation validate_directions(std::string_view path);

//! Call validate_directions() only when running in debug mode (log_level >= 3).
//! Hard errors are caught and re-emitted as DEBUG messages.
//! Intended for use in commands that consume direction files.
void debug_validate_directions(std::string_view path);

} // namespace MR::DWI::Directions
