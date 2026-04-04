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

#include "header.h"
#include "types.h"

namespace MR::DWI::Tractography::ACT {

//! Maximum permitted absolute deviation from a unity partial-volume sum.
//! Brain voxels where |sum(PVF) - 1| exceeds this threshold are counted
//! as soft violations (non-unity-sum voxels).
constexpr default_type max_sum_deviation = 1e-3;

//! Summary of a full 5TT image validation.
struct FiveTTValidation {

  //! Number of brain voxels where any individual tissue partial volume
  //! fraction (PVF) lies outside the physical range [0.0, 1.0].
  //! A non-zero value is a hard violation.
  size_t n_voxels_abs_error = 0;

  //! Number of brain voxels where the sum of the five tissue partial volume
  //! fractions deviates from 1.0 by more than max_sum_deviation.
  //! A non-zero value is a soft violation: the image may still be usable
  //! for ACT but does not perfectly conform to the format.
  size_t n_voxels_sum_error = 0;
};

//! Validate a 5TT image and return a summary of findings.
//!
//! Checks performed:
//!
//!   1. Structural validity: the image must be of floating-point type,
//!      4-dimensional, and contain exactly 5 volumes.
//!      Failure throws an Exception immediately (via verify_5TT_image()).
//!
//!   2. Per-voxel absolute range: for every brain voxel (non-zero PVF sum),
//!      each of the five tissue partial volume fractions must lie within
//!      [0.0, 1.0].  Violations are counted in n_voxels_abs_error.
//!
//!   3. Per-voxel unity sum: for every brain voxel, the sum of all five
//!      tissue partial volume fractions must equal 1.0 within
//!      max_sum_deviation.  Violations are counted in n_voxels_sum_error.
//!
//! Only structural violations (check 1) cause an Exception to be thrown.
//! Violations in checks 2 and 3 are returned as counts so that the caller
//! can decide how to handle them (hard error vs. warning).
FiveTTValidation validate_5TT(const Header &H);

//! Call validate_5TT() only when running in debug mode (log_level >= 3).
//! Structural exceptions are caught and re-emitted as DEBUG messages.
//! Content violations (abs / sum errors) are also emitted as DEBUG messages.
//! Intended for use in commands that consume 5TT images.
void debug_validate_5TT(const Header &H);

} // namespace MR::DWI::Tractography::ACT
