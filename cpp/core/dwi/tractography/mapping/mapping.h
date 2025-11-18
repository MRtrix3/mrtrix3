/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include "progressbar.h"
#include "types.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

namespace MR::DWI::Tractography::Mapping {

// Default angular threshold for assigning streamlines to fixels
//   in the absence of any other information
constexpr default_type default_streamline2fixel_angle = 45.0;

// Didn't bother making this a command-line option,
//   since curvature contrast results were very poor regardless of smoothing
constexpr default_type curvature_smoothing_mm = 10.0;

// How many streamlines to read from a tractogram file
//   in order to establish the spatial bounding box
constexpr ssize_t streamlines_for_bounding_box = 1000000;

// Convenience functions to figure out an appropriate upsampling ratio for streamline mapping
size_t determine_upsample_ratio(const Header &, const float, const float);
size_t determine_upsample_ratio(const Header &, std::string_view, const float);
size_t determine_upsample_ratio(const Header &, const Tractography::Properties &, const float);

void generate_header(Header &, std::string_view, const std::vector<default_type> &);

void oversample_header(Header &, const std::vector<default_type> &);

} // namespace MR::DWI::Tractography::Mapping
