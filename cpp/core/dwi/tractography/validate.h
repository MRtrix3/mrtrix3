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
#include <string>
#include <string_view>

namespace MR::DWI::Tractography {

//! Summary of a full tractogram validation.
struct TckValidation {

  //! True if the "count" field is present in the file header.
  bool has_header_count = false;

  //! Value of the "count" field as stored in the header (only valid when has_header_count is true).
  size_t header_count = 0;

  //! Number of streamlines actually present in the file (i.e. number of NaN delimiters seen).
  size_t n_streamlines = 0;

  //! Number of streamlines that contain zero vertices.
  size_t n_empty = 0;

  //! Number of streamlines that contain exactly one vertex.
  size_t n_single_vertex = 0;
};

//! Validate a tractogram (.tck) file and return a summary of findings.
//!
//! Every 3-float triplet in the binary data section is classified as one of:
//!   - All-finite:  a regular vertex belonging to the current streamline.
//!   - All-NaN:     the inter-streamline delimiter that terminates the current streamline.
//!   - All-infinity:the mandatory end-of-file barrier, which must be the last triplet.
//!
//! Any other triplet (partial NaN, partial infinity, or a mix) is considered invalid.
//!
//! Throws Exception on the first of these hard violations:
//!   1. An invalid (partially non-finite) triplet is found.
//!   2. Vertices are present after the end-of-file barrier.
//!   3. A streamline body is open (vertices accumulated with no terminating
//!      delimiter) when the end-of-file barrier is reached.
//!   4. The binary data section ends without an end-of-file barrier.
//!   5. The "count" field is absent from the header.
//!   6. The "count" field does not match the number of streamlines read.
//!
//! The returned TckValidation struct reports n_empty and n_single_vertex for
//! downstream reporting; these are not treated as hard errors.
TckValidation validate_tck(std::string_view path);

//! Call validate_tck() only when running in debug mode (log_level >= 3).
//! Hard errors are re-thrown; soft findings are emitted as DEBUG messages.
//! Intended for use in tractography-processing commands.
void debug_validate_tck(std::string_view path);

//! Validate a track scalar file (.tsf) against its corresponding tractogram (.tck).
//!
//! Throws Exception on any of the following:
//!   1. "timestamp" field is absent from either header, or timestamps do not match —
//!      indicating the TSF was not produced from the given tractogram.
//!   2. "count" field is absent from the TSF header.
//!   3. The "count" field in the TSF header does not match the number of scalar
//!      sequences actually present in the TSF file.
//!   4. The number of scalar sequences in the TSF does not match the number of
//!      streamlines in the tractogram.
//!   5. One or more streamlines have a different number of vertices in the TSF
//!      and in the tractogram.
void validate_tsf(std::string_view tsf_path, std::string_view tck_path);

//! Call validate_tsf() only when running in debug mode (log_level >= 3).
//! Hard errors are re-thrown as DEBUG messages.
//! Intended for use in track-scalar-processing commands.
void debug_validate_tsf(std::string_view tsf_path, std::string_view tck_path);

} // namespace MR::DWI::Tractography
