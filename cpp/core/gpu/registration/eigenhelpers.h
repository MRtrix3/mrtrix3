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

#include "types.h"

#include <Eigen/Core>

#include <array>
#include <string>

namespace MR::EigenHelpers {

Eigen::Matrix4f to_homogeneous_mat4f(const transform_type &source_transform);
Eigen::Matrix4d to_homogeneous_mat4d(const transform_type &source_transform);

transform_type from_homogeneous_mat4f(const Eigen::Matrix4f &source_matrix);
transform_type from_homogeneous_mat4d(const Eigen::Matrix4d &source_matrix);

std::array<float, 16> to_array(const Eigen::Matrix4f &matrix);
std::array<float, 16> to_array(const transform_type &transform);
std::array<float, 3> to_array(const Eigen::Vector3f &vector);

Eigen::Vector3f to_vector3f(const std::array<float, 3> &array);

/// Returns a 4x4 homogeneous scaling matrix for the given scale factor.
Eigen::Matrix4f make_scaling_mat4f(float scale_factor);

std::string to_string(const Eigen::Matrix4f &matrix);

} // namespace MR::EigenHelpers
