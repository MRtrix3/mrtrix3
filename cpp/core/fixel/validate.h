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

#include <optional>
#include <string_view>

#include "fixel/fixel.h"

namespace MR {
class Header;
template <typename ValueType> class Image;
} // namespace MR

namespace MR::Fixel {

//! Perform thorough validation of a fixel directory.
//! Checks that:
//!   - a valid index image is present,
//!     and every fixel index is covered by exactly one voxel in the index image
//!     (via validate_index_image())
//!   - a valid directions image is present;
//!   - all fixel data files contain the same number of fixels as the index image.
//! Throws InvalidDirectoryException on any failure.
void validate_directory(std::string_view fixel_directory_path);

//! Perform validation of a fixel index image.
//! Checks that:
//!   - a valid index image is present,
//!     and every fixel index is covered by exactly one voxel in the index image
//! Returns the total number of fixels in the index image
index_type validate_index_image(Image<index_type> index_image);

//! Call validate() only when running in debug mode (log_level >= 3).
//! Intended for use in other fixel commands to add lightweight validation in debug builds.
void debug_validate_directory(std::string_view fixel_directory_path);

//! Call validate_debug_index_image() only when running in debug mode (log_level >= 3).
//! Intended for use in other fixel commands to add lightweight validation in debug builds.
void debug_validate_index_image(const Image<index_type> &index_image);

} // namespace MR::Fixel

namespace MR::Peaks {

//! Outcome of a full peaks-image content validation.
struct PeaksValidation {

  //! Fill value used when a voxel has fewer peaks than the maximum.
  //! Either 0.0 or NaN if present;
  //! has no value when no fill triplets were encountered
  //! (i.e. every voxel is fully populated).
  std::optional<float> fill_value = std::nullopt;

  //! Minimum norm across all non-fill (real) peaks.
  float norm_min = std::numeric_limits<float>::quiet_NaN();

  //! Maximum norm across all non-fill (real) peaks.
  float norm_max = std::numeric_limits<float>::quiet_NaN();
};

//! Validate that a header can be interpreted as a peaks image.
//!
//! The following checks are performed in order:
//!   1. Floating-point datatype.
//!   2. Effective 4-dimensionality with number of volumes a multiple of 3.
//!
//! Throws Exception with a descriptive message on the first violation.
void validate_header(const Header &H);

//! Validate the content of a peaks image, returning a PeaksValidation summary.
//!
//! Additional checks over and above validate_header():
//!   3. Fill value consistency: every fill triplet uses the same convention
//!      (all-zero or all-NaN), and both conventions must not be mixed.
//!   4. NaN-fill coherence: when the fill value is NaN, every triplet must
//!      be either entirely finite or entirely NaN — partial NaN triplets are
//!      forbidden.
//!   5. Infinity values are not permitted.
//!
//! Throws Exception with a descriptive message on the first violation.
//! The PeaksValidation summary is populated for all non-throwing paths.
[[nodiscard]] const PeaksValidation validate_image(Image<float> image);

//! Call validate_peaks_image() only when running in debug mode (log_level >= 3).
//! Format errors are always re-thrown; content findings are emitted as DEBUG messages.
//! Intended for use in peaks-processing commands.
void debug_validate_image(const Image<float> &image);

} // namespace MR::Peaks
