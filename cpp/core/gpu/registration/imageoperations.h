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

#include "gpu/gpu.h"
#include <tcb/span.hpp>
#include "transform.h"
#include "types.h"

#include <Eigen/Core>
#include <array>
#include <optional>
#include <vector>

namespace MR {
template <class ImageType, class ValueType = default_type>
Eigen::Matrix<ValueType, 3, 1> image_centre_scanner_space(const ImageType &image) {
  const ValueType half = static_cast<ValueType>(0.5);
  const ValueType one = static_cast<ValueType>(1.0);
  Eigen::Matrix<ValueType, 3, 1> centre_voxel;
  centre_voxel[0] = static_cast<ValueType>(image.size(0)) * half - one;
  centre_voxel[1] = static_cast<ValueType>(image.size(1)) * half - one;
  centre_voxel[2] = static_cast<ValueType>(image.size(2)) * half - one;
  const Transform transform(image);
  return transform.voxel2scanner.template cast<ValueType>() * centre_voxel;
}
} // namespace MR

namespace MR::GPU {
// Compute center of mass of a given image the image
std::array<float, 3> centerOfMass(const GPU::Texture &texture,
                                  const GPU::ComputeContext &context,
                                  const transform_type &imageTransform = transform_type::Identity(),
                                  std::optional<GPU::Texture> mask = std::nullopt);

Eigen::Matrix3f computeScannerMoments(const GPU::Texture &texture,
                                      const GPU::ComputeContext &context,
                                      const Eigen::Matrix4f &voxelToScanner,
                                      const Eigen::Vector3f &centreScanner,
                                      std::optional<GPU::Texture> mask = std::nullopt);

// Transform the image using the given transformation
// If you want to transform an image in scanner coordinates
// then this transformation must be equal =
// scanner to voxel mat * transformation * voxel to scanner mat
GPU::Texture transformTexture(const GPU::Texture &texture,
                              const GPU::ComputeContext &context,
                              tcb::span<const float> transformationMatrixData);

GPU::Texture downsampleTexture(const GPU::Texture &texture, const GPU::ComputeContext &context);

std::vector<GPU::Texture>
createDownsampledPyramid(const GPU::Texture &fullResTexture, int numLevels, const GPU::ComputeContext &context);
} // namespace MR::GPU
