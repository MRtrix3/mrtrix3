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

#include <cmath>
#include <optional>

#include "header.h"

namespace MR::Registration::Warp {

//! The warp format inferred from the image dimensions.
enum class WarpFormat {
  //! 4D image with 3 volumes: a displacement or deformation field.
  Simple,
  //! 5D image with 3 × 4 volumes and header-embedded linear transforms.
  Full
};

//! Summary of a full warp-image validation.
struct WarpValidation {

  //! The warp format inferred from the image dimensions.
  WarpFormat format = WarpFormat::Simple;

  //! The fill value convention in use (0.0 = zero fill, NaN = NaN fill).
  std::optional<float> fill_value = std::nullopt;
};

//! Validate that a header conforms to expectations of a non-linear warp image.
//!
//! Checks performed:
//!
//!   1. The image data type must be floating-point.
//!
//!   2. For a displacement or deformation field the image must be 4D
//!      with exactly 3 volumes in the 4th dimension.
//!      For a full warp field the image must be 5D with exactly 3 volumes in
//!      the 4th dimension (the x/y/z displacement components) and exactly 4
//!      volumes in the 5th dimension (the four warp components: image1-to-midway,
//!      midway-to-image1, image2-to-midway, midway-to-image2).
//!      Any other dimensionality is a hard error.
//!
//!   3. For full warp fields, the header must contain both a "linear1" and a
//!      "linear2" key encoding the linear transforms for each image.

// TODO Add std::optional to demand one of the two formats
// Once implemented, can remove multiple redundant checks in individual commands

WarpFormat validate_header(const Header &H);

//! Validate a non-linear warp image and return a summary of findings.
//!
//! Checks performed in addition to those covered by validate_header():
//!
//!   4. At each spatial voxel, the warp is represented as a triplet of three
//!      floating-point values (one per volume group for full warps).
//!      A triplet may be a fill value, signalling that the voxel lies outside
//!      the domain of the warp.  The fill convention must be consistent across
//!      the entire image: either all fill triplets are all-NaN, or all fill
//!      triplets are some fixed value; both conventions must not be mixed.
//!
//!   5. Every triplet must be either entirely finite or entirely NaN.
//!      Triplets that are partly NaN and partly finite are not valid.
//!
//!   6. Infinity values are not permitted.
//!
//! Throws Exception on the first hard violation.
template <typename ValueType> WarpValidation validate_image(Image<ValueType> image);

//! Call validate_header(),
//! but only additionally execute validate_image() when running in debug mode (log_level >= 3).
//! Hard errors are caught and re-emitted as DEBUG messages.
//! Intended for use in commands that consume non-linear warp images.
template <typename ValueType> void debug_validate_image(Image<ValueType> image);

} // namespace MR::Registration::Warp
