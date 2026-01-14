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

#include "gpu/registration/eigenhelpers.h"
#include "image.h"
#include "transform.h"
#include <array>

namespace MR::GPU {

// This struct provides 4x4 matrices for converting between voxel and scanner spaces
// for both moving and fixed images. It's designed for use in GPU buffers.

struct alignas(16) VoxelScannerMatrices {
  std::array<float, 16> voxel_to_scanner_moving;
  std::array<float, 16> voxel_to_scanner_fixed;
  std::array<float, 16> scanner_to_voxel_moving;
  std::array<float, 16> scanner_to_voxel_fixed;

  static VoxelScannerMatrices from_image_pair(const Image<float> &moving, const Image<float> &fixed,
                                            float scale_factor = 1.0F) {
    const Eigen::Matrix4f scale_matrix = EigenHelpers::make_scaling_mat4f(scale_factor);

    const auto moving_transform = MR::Transform(moving);
    const auto fixed_transform = MR::Transform(fixed);

    const Eigen::Matrix4f voxel_to_scanner_moving_mat =
        EigenHelpers::to_homogeneous_mat4f(moving_transform.voxel2scanner) * scale_matrix;
    const Eigen::Matrix4f voxel_to_scanner_fixed_mat =
        EigenHelpers::to_homogeneous_mat4f(fixed_transform.voxel2scanner) * scale_matrix;

    const Eigen::Matrix4f scanner_to_voxel_moving_mat = voxel_to_scanner_moving_mat.inverse();
    const Eigen::Matrix4f scanner_to_voxel_fixed_mat = voxel_to_scanner_fixed_mat.inverse();

    return VoxelScannerMatrices{.voxel_to_scanner_moving = EigenHelpers::to_array(voxel_to_scanner_moving_mat),
                                .voxel_to_scanner_fixed = EigenHelpers::to_array(voxel_to_scanner_fixed_mat),
                                .scanner_to_voxel_moving = EigenHelpers::to_array(scanner_to_voxel_moving_mat),
                                .scanner_to_voxel_fixed = EigenHelpers::to_array(scanner_to_voxel_fixed_mat)};
  }
};

} // namespace MR::GPU
