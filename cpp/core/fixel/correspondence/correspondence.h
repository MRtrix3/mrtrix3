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

#include "fixel/fixel.h"

#define FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
// #define FIXELCORRESPONDENCE_TEST_COMBINATORICS

namespace MR::Fixel::Correspondence {

using index_type = MR::Fixel::index_type;
using dir_t = Eigen::Matrix<float, 3, 1>;
using voxel_t = Eigen::Array<uint32_t, 3, 1>;

/// @brief Integer type used for CSR indptr and indices arrays in .npz correspondence files.
using npz_index_type = uint32_t;
/// @brief Value type used for CSR data arrays in .npz correspondence files.
using npz_value_type = float;

constexpr index_type min_dirs_to_enforce_adjacency = 4;
constexpr index_type max_fixels_for_no_combinatorial_warning = 6;
constexpr unsigned int dp2cost_lookup_resolution = 1000;

constexpr float default_in2023_alpha = 0.5f;
constexpr float default_in2023_beta = 0.1f;
constexpr float default_pot_p = 1.0f;
constexpr float default_pot_gamma = 0.5f;
constexpr float default_nearest_maxangle = 45.0f;

constexpr index_type default_max_origins_per_target = 3;
constexpr index_type default_max_objectives_per_source = 3;

} // namespace MR::Fixel::Correspondence
