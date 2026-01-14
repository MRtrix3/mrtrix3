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

#include "gpu/registration/imageoperations.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/gpu.h"
#include <tcb/span.hpp>
#include "types.h"
#include "gpu/registration/utils.h"

#include <Eigen/Core>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace MR::GPU {
using Coordinate3D = std::array<float, 3>;

struct MomentUniforms {
  alignas(16) std::array<float, 4> centre{};
};
static_assert(sizeof(MomentUniforms) % 16 == 0, "MomentUniforms must be 16-byte aligned");

Coordinate3D centerOfMass(const Texture &texture,
                          const ComputeContext &context,
                          const transform_type &imageTransform,
                          std::optional<Texture> mask) {
  const WorkgroupSize workgroupSize{.x = 8, .y = 8, .z = 4};

  const Buffer<uint32_t> weightedPositionBuffer = context.new_empty_buffer<uint32_t>(3);
  const Buffer<float> totalWeightBuffer = context.new_empty_buffer<float>(1);

  const KernelSpec centerOfMassKernelSpec{
      .compute_shader =
          {
              .shader_source = ShaderFile{"shaders/center_of_mass.slang"},
              .workgroup_size = workgroupSize,
              .constants = {{"kUseMask", static_cast<uint32_t>(mask.has_value())}},
          },
      .bindings_map = {{"weightedPositions", weightedPositionBuffer},
                       {"totalIntensity", totalWeightBuffer},
                       {"image", texture},
                       {"mask", mask ? *mask : texture}},
  };

  const Kernel centerOfMassKernel = context.new_kernel(centerOfMassKernelSpec);
  const DispatchGrid dispatch_grid = DispatchGrid::element_wise_texture(texture, workgroupSize);
  context.dispatch_kernel(centerOfMassKernel, dispatch_grid);

  std::array<uint32_t, 3> weightedPositionValues{};
  uint32_t totalWeightValue;

  context.download_buffer<uint32_t>(weightedPositionBuffer, weightedPositionValues);
  context.download_buffer(totalWeightBuffer, &totalWeightValue, sizeof(float));

  // Now reinterpret the downloaded data as float
  std::array<float, 3> weightedPosition;
  weightedPosition[0] = *reinterpret_cast<float *>(&weightedPositionValues[0]);
  weightedPosition[1] = *reinterpret_cast<float *>(&weightedPositionValues[1]);
  weightedPosition[2] = *reinterpret_cast<float *>(&weightedPositionValues[2]);

  const float totalWeight = *reinterpret_cast<float *>(&totalWeightValue);

  const auto center = Eigen::Vector4f(
      weightedPosition[0] / totalWeight, weightedPosition[1] / totalWeight, weightedPosition[2] / totalWeight, 1.0F);

  assert(center.x() >= 0 && center.x() <= texture.spec.width && center.y() >= 0 && center.y() <= texture.spec.height &&
         center.z() >= 0 && center.z() <= texture.spec.depth && "Center of mass is out of the bounds of the image");

  const auto centerScanner = imageTransform.matrix().cast<float>() * center;

  return {centerScanner.x(), centerScanner.y(), centerScanner.z()};
}

Eigen::Matrix3f computeScannerMoments(const Texture &texture,
                                      const ComputeContext &context,
                                      const Eigen::Matrix4f &voxelToScanner,
                                      const Eigen::Vector3f &centreScanner,
                                      std::optional<Texture> mask) {
  constexpr size_t kMomentCount = 6;
  const WorkgroupSize workgroupSize{.x = 8, .y = 8, .z = 4};

  const std::array<float, 16> matrixData = EigenHelpers::to_array(voxelToScanner);
  const Buffer<float> matrixBuffer = context.new_buffer_from_host_memory<float>(matrixData);

  const MomentUniforms uniforms{
      .centre = {centreScanner.x(), centreScanner.y(), centreScanner.z(), 0.0f},
  };
  const Buffer<std::byte> centreBuffer =
      context.new_buffer_from_host_memory<std::byte>(&uniforms, sizeof(uniforms), BufferType::UniformBuffer);

  Buffer<uint32_t> momentBuffer = context.new_empty_buffer<uint32_t>(kMomentCount);
  context.clear_buffer(momentBuffer);

  const KernelSpec kernelSpec{
      .compute_shader = {.shader_source = ShaderFile{"shaders/registration/moments.slang"},
                         .workgroup_size = workgroupSize,
                         .constants = {{"kUseMask", static_cast<uint32_t>(mask.has_value())}}},
      .bindings_map = {{"momentBuffer", momentBuffer},
                       {"voxelToScanner", matrixBuffer},
                       {"centreScanner", centreBuffer},
                       {"image", texture},
                       {"mask", mask ? *mask : texture}},
  };

  const Kernel kernel = context.new_kernel(kernelSpec);
  const DispatchGrid dispatch_grid = DispatchGrid::element_wise_texture(texture, workgroupSize);

  context.dispatch_kernel(kernel, dispatch_grid);

  std::array<uint32_t, kMomentCount> momentBits{};
  context.download_buffer<uint32_t>(momentBuffer, momentBits);

  std::array<float, kMomentCount> momentValues{};
  for (size_t i = 0; i < kMomentCount; ++i) {
    std::memcpy(&momentValues[i], &momentBits[i], sizeof(float));
  }

  Eigen::Matrix3f moments;
  moments << momentValues[0], momentValues[3], momentValues[4], momentValues[3], momentValues[1], momentValues[5],
      momentValues[4], momentValues[5], momentValues[2];
  return moments;
}

Texture transformTexture(const Texture &texture,
                         const ComputeContext &context,
                         tcb::span<const float> transformationMatrixData) {
  const WorkgroupSize workgroupSize{.x = 8, .y = 8, .z = 4};

  const Buffer<float> transformationMatrixBuffer = context.new_buffer_from_host_memory(transformationMatrixData);

  const TextureSpec outputTextureSpec{.width = texture.spec.width,
                                      .height = texture.spec.height,
                                      .depth = texture.spec.depth,
                                      .format = texture.spec.format,
                                      .usage = {.storage_binding = true, .render_target = false}};
  const Texture outputTexture = context.new_empty_texture(outputTextureSpec);

  const KernelSpec transformKernelSpec{
      .compute_shader = {.shader_source = ShaderFile{"shaders/transform_image.slang"},
                         .workgroup_size = workgroupSize},
      .bindings_map = {{"transformationMatrix", transformationMatrixBuffer},
                       {"inputImage", texture},
                       {"outputImage", outputTexture},
                       {"linearSampler", context.new_linear_sampler()}}};

  const Kernel transformKernel = context.new_kernel(transformKernelSpec);
  const DispatchGrid dispatch_grid{
      .x = Utils::nextMultipleOf(texture.spec.width / workgroupSize.x, workgroupSize.x),
      .y = Utils::nextMultipleOf(texture.spec.height / workgroupSize.y, workgroupSize.y),
      .z = Utils::nextMultipleOf(texture.spec.depth / workgroupSize.z, workgroupSize.z),
  };

  context.dispatch_kernel(transformKernel, dispatch_grid);

  return outputTexture;
}

Texture downsampleTexture(const Texture &texture, const ComputeContext &context) {
  const WorkgroupSize workgroupSize{.x = 8, .y = 8, .z = 4};

  const TextureSpec outputTextureSpec{.width = texture.spec.width / 2,
                                      .height = texture.spec.height / 2,
                                      .depth = texture.spec.depth / 2,
                                      .format = texture.spec.format,
                                      .usage = {.storage_binding = true, .render_target = false}};
  const Texture outputTexture = context.new_empty_texture(outputTextureSpec);

  const KernelSpec transformKernelSpec{
      .compute_shader = {.shader_source = ShaderFile{"shaders/downsample_image.slang"},
                         .workgroup_size = workgroupSize},
      .bindings_map = {{"inputTexture", texture}, {"outputTexture", outputTexture}}};
  const Kernel transformKernel = context.new_kernel(transformKernelSpec);
  const DispatchGrid dispatch_grid{
      .x = Utils::nextMultipleOf(outputTextureSpec.width / workgroupSize.x, workgroupSize.x),
      .y = Utils::nextMultipleOf(outputTextureSpec.height / workgroupSize.y, workgroupSize.y),
      .z = Utils::nextMultipleOf(outputTextureSpec.depth / workgroupSize.z, workgroupSize.z),
  };

  context.dispatch_kernel(transformKernel, dispatch_grid);

  return outputTexture;
}

std::vector<Texture>
createDownsampledPyramid(const Texture &fullResTexture, int32_t numLevels, const ComputeContext &context) {
  if (numLevels == 0)
    return {};

  std::vector<Texture> pyramid(numLevels);
  pyramid[numLevels - 1] = fullResTexture;

  for (int level = static_cast<int>(numLevels) - 2; level >= 0; --level) {
    pyramid[level] = downsampleTexture(pyramid[level + 1], context);
  }
  return pyramid;
}

} // namespace MR::GPU
