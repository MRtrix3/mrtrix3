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

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "connectome/connectome.h"
#include "header.h"

namespace MR::Connectome {

//! Results from a full validation of a label (hard segmentation) image.
struct LabelValidation {

  //! The set of unique non-background (non-zero) label values present in the image
  std::vector<node_t> labels;

  //! True if the non-zero labels form the unbroken range {1, 2, ..., max_label}
  bool indices_contiguous = true;

  //! Any values in [1, max_label] that are absent from the image
  std::vector<node_t> missing_indices;

  //! For each non-background label, the number of spatially connected components
  //! under 26-nearest-neighbour voxel connectivity
  std::map<node_t, uint32_t> component_counts;

  //! The total number of non-background labels that have more than one component
  node_t disconnected_components = 0;
};

//! Validate a hard segmentation (label) image header.
//!
//! Checks that the image is 3D (or 4D with a singleton 4th dimension) and
//! contains only non-negative integer values, throwing Exception on failure.
void validate_label_header(const Header &H);

//! Validate content of a hard segmentation (label) image.
//! In addition to those checks performed by validate_label_header(),
//! additionally analyses index contiguity and, per label,
//! spatial contiguity under 26-nearest-neighbour voxel connectivity.
[[nodiscard]] const LabelValidation validate_label_image(Image<node_t> image);

//! Call validate_label_image() only when running in debug mode (log_level >= 3).
//! Format errors are re-thrown; contiguity findings are reported as DEBUG messages.
//! Intended for use in label-processing commands to add validation in debug builds.
void debug_validate_label_image(const Image<node_t> &image);

} // namespace MR::Connectome
