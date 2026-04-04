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

#include "header.h"

namespace MR::Peaks {

//! Outcome of a full peaks-image content validation.
struct PeaksValidation {

  //! Fill value used when a voxel has fewer peaks than the maximum.
  //! Either 0.0 or NaN; indeterminate (set to 0.0) when no fill triplets
  //! were encountered (i.e. every voxel is fully populated).
  float fill_value = 0.0F;

  //! True once at least one fill triplet has been observed.
  bool fill_value_seen = false;

  //! Minimum norm across all non-fill (real) peaks.
  float norm_min = 0.0F;

  //! Maximum norm across all non-fill (real) peaks.
  float norm_max = 0.0F;

  //! True when at least one non-fill peak has been observed.
  bool any_peaks_seen = false;
};

//! Validate the content of a peaks image, returning a PeaksValidation summary.
//!
//! The following checks are performed in order:
//!   1. Floating-point datatype.
//!   2. Effective 4-dimensionality with number of volumes a multiple of 3.
//!   3. Fill value consistency: every fill triplet uses the same convention
//!      (all-zero or all-NaN), and both conventions must not be mixed.
//!   4. NaN-fill coherence: when the fill value is NaN, every triplet must
//!      be either entirely finite or entirely NaN — partial NaN triplets are
//!      forbidden.
//!   5. All non-fill triplets must have finite, strictly positive norms.
//!
//! Throws Exception with a descriptive message on the first violation.
//! The PeaksValidation summary is populated for all non-throwing paths.
PeaksValidation validate_peaks_image(const Header &H);

//! Call validate_peaks_image() only when running in debug mode (log_level >= 3).
//! Format errors are always re-thrown; content findings are emitted as DEBUG messages.
//! Intended for use in peaks-processing commands.
void debug_validate_peaks_image(const Header &H);

} // namespace MR::Peaks
