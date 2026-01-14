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

#include "gpu/registration/nmicalculator.h"

#include "gpu/registration/calculatoroutput.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/gpu.h"
#include "gpu/registration/registrationtypes.h"
#include "gpu/registration/utils.h"
#include "gpu/registration/voxelscannermatrices.h"

#include <Eigen/Core>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>

namespace MR::GPU {
namespace {
uint32_t float_to_ordered_uint(float v) {
  uint32_t bits;
  // Use std::bit_cast when we switch to C++20
  std::memcpy(&bits, &v, sizeof(bits));
  return (bits & 0x80000000u) ? ~bits : (bits ^ 0x80000000u);
}

float ordered_uint_to_float(uint32_t v) {
  const uint32_t bits = (v & 0x80000000u) ? (v ^ 0x80000000u) : ~v;
  float out;
  // Use std::bit_cast when we switch to C++20
  std::memcpy(&out, &bits, sizeof(out));
  return out;
}
} // namespace

struct MinMaxUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
};
static_assert(sizeof(MinMaxUniforms) % 16 == 0, "MinMaxUniforms must be 16-byte aligned");

struct JointHistogramUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
  alignas(16) Intensities intensities{};
  alignas(16) std::array<float, 16> transformation_matrix{};
};
static_assert(sizeof(JointHistogramUniforms) % 16 == 0, "JointHistogramUniforms must be 16-byte aligned");

struct PrecomputeUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
};
static_assert(sizeof(PrecomputeUniforms) % 16 == 0, "PrecomputeUniforms must be 16-byte aligned");

template <size_t N> struct GradientsUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
  alignas(16) std::array<float, 3> transformation_pivot{};
  alignas(16) Intensities intensities{};
  alignas(16) std::array<float, N> current_transform{};
  alignas(16) VoxelScannerMatrices voxel_scanner_matrices{};
};

using RigidGradientsUniforms = GradientsUniforms<6>;
using AffineGradientsUniforms = GradientsUniforms<12>;

constexpr WorkgroupSize gradientsWorkgroupSize{16, 8, 8};
const std::array<uint32_t, 2> initialMinMax{
    float_to_ordered_uint(std::numeric_limits<float>::max()),
    float_to_ordered_uint(-std::numeric_limits<float>::max()),
};

// Order of operations to drive GPU computation:
// 1. Find the min/max intensities of the fixed image and moving image (with current transformation applied)
// 2. Compute the joint histogram of the fixed and moving images
// 3. Precompute a coefficients table from the joint histogram to avoid redundant computations in the next stage and
//    compute the mutual information cost.
// 4. Compute the gradients of the mutual information cost function with respect to the transformation parameters.
NMICalculator::NMICalculator(const Config &config)
    : m_output(config.output),
      m_compute_context(config.context),
      m_fixed(config.fixed),
      m_moving(config.moving),
      m_fixed_mask(config.fixed_mask.value_or(config.fixed)),
      m_moving_mask(config.moving_mask.value_or(config.moving)),
      m_use_fixed_mask(config.fixed_mask.has_value()),
      m_use_moving_mask(config.moving_mask.has_value()),
      m_voxel_scanner_matrices(config.voxel_scanner_matrices),
      m_num_bins(config.num_bins)

{
  assert(m_compute_context != nullptr);
  const bool is_rigid = config.transformation_type == TransformationType::Rigid;
  const bool is_affine = config.transformation_type == TransformationType::Affine;
  m_degrees_of_freedom = is_rigid ? 6 : 12;

  // The min/max reduction runs on encoded uint32_t values. We map floats to an order-preserving
  // uint representation (flip sign bit for positives, bitwise-not for negatives), so unsigned
  // comparisons match float ordering and atomics work for negative intensities too.
  // TODO: Should we use shared memory reduction instead of atomics for better performance?
  m_min_max_uniforms_buffer =
      m_compute_context->new_empty_buffer<std::byte>(sizeof(MinMaxUniforms), BufferType::UniformBuffer);
  m_min_max_intensity_fixed_buffer =
      m_compute_context->new_buffer_from_host_memory<uint32_t>(initialMinMax.data(), sizeof(initialMinMax));
  m_min_max_intensity_moving_buffer =
      m_compute_context->new_buffer_from_host_memory<uint32_t>(initialMinMax.data(), sizeof(initialMinMax));
  m_raw_joint_histogram_buffer = m_compute_context->new_empty_buffer<uint32_t>(m_num_bins * m_num_bins);
  m_smoothed_joint_histogram_buffer = m_compute_context->new_empty_buffer<float>(m_num_bins * m_num_bins);
  m_joint_histogram_mass_buffer = m_compute_context->new_empty_buffer<float>(1);
  m_joint_histogram_uniforms_buffer =
      m_compute_context->new_empty_buffer<std::byte>(sizeof(JointHistogramUniforms), BufferType::UniformBuffer);
  m_precomputed_coefficients_buffer = m_compute_context->new_empty_buffer<float>(m_num_bins * m_num_bins);
  m_mutual_information_buffer = m_compute_context->new_empty_buffer<float>(1);

  if (m_output == CalculatorOutput::CostAndGradients) {
    m_gradients_dispatch_grid = DispatchGrid::element_wise_texture(m_fixed, gradientsWorkgroupSize);
    const uint32_t gradients_uniform_size =
        is_affine ? sizeof(AffineGradientsUniforms) : sizeof(RigidGradientsUniforms);
    m_gradients_uniforms_buffer =
        m_compute_context->new_empty_buffer<std::byte>(gradients_uniform_size, BufferType::UniformBuffer);
    m_gradients_buffer =
        m_compute_context->new_empty_buffer<float>(m_degrees_of_freedom * m_gradients_dispatch_grid.workgroup_count());
  }

  const KernelSpec min_max_fixed_kernel_spec{
      .compute_shader = {.shader_source = ShaderFile{"shaders/reduction_image.slang"},
                         .entryPoint = "minMaxAtomic"},
      .bindings_map = {{"uniforms", m_min_max_uniforms_buffer},
                       {"inputTexture", m_fixed},
                       {"outputBuffer", m_min_max_intensity_fixed_buffer},
                       {"sampler", m_compute_context->new_linear_sampler()}},
  };
  const auto min_max_fixed_kernel = m_compute_context->new_kernel(min_max_fixed_kernel_spec);
  const DispatchGrid fixed_dispatch_grid =
      DispatchGrid::element_wise_texture(m_fixed, min_max_fixed_kernel.workgroup_size);
  const MinMaxUniforms min_max_fixed_uniforms{
      .dispatch_grid = fixed_dispatch_grid,
  };
  m_compute_context->write_to_buffer(m_min_max_uniforms_buffer, &min_max_fixed_uniforms, sizeof(min_max_fixed_uniforms));
  m_compute_context->dispatch_kernel(min_max_fixed_kernel, fixed_dispatch_grid);

  const KernelSpec min_max_moving_kernel_spec{
      .compute_shader = {.shader_source = ShaderFile{"shaders/reduction_image.slang"},
                         .entryPoint = "minMaxAtomic"},
      .bindings_map = {
          {"uniforms", m_min_max_uniforms_buffer},
          {"inputTexture", m_moving},
          {"outputBuffer", m_min_max_intensity_moving_buffer},
          {"sampler", m_compute_context->new_linear_sampler()},
      }};

  m_min_max_moving_kernel = m_compute_context->new_kernel(min_max_moving_kernel_spec);
  const DispatchGrid moving_dispatch_grid =
      DispatchGrid::element_wise_texture(m_moving, m_min_max_moving_kernel.workgroup_size);
  const MinMaxUniforms min_max_moving_uniforms{
      .dispatch_grid = moving_dispatch_grid,
  };
  m_compute_context->write_to_buffer(m_min_max_uniforms_buffer, &min_max_moving_uniforms, sizeof(MinMaxUniforms));
  m_compute_context->dispatch_kernel(m_min_max_moving_kernel, moving_dispatch_grid);

  const std::vector<uint32_t> min_max_fixed_bits =
      m_compute_context->download_buffer_as_vector(m_min_max_intensity_fixed_buffer);
  const std::vector<uint32_t> min_max_moving_bits =
      m_compute_context->download_buffer_as_vector(m_min_max_intensity_moving_buffer);

  m_intensities = {ordered_uint_to_float(min_max_moving_bits[0]),
                   ordered_uint_to_float(min_max_moving_bits[1]),
                   ordered_uint_to_float(min_max_fixed_bits[0]),
                   ordered_uint_to_float(min_max_fixed_bits[1])};

  const WorkgroupSize joint_histogram_wg_size = {8, 8, 4};

  m_joint_histogram_dispatch_grid = DispatchGrid::element_wise_texture(m_fixed, joint_histogram_wg_size);
  const JointHistogramUniforms joint_histogram_uniforms{
      .dispatch_grid = m_joint_histogram_dispatch_grid,
      .intensities = m_intensities,
      .transformation_matrix = {},
  };
  m_compute_context->write_to_buffer(
      m_joint_histogram_uniforms_buffer, &joint_histogram_uniforms, sizeof(JointHistogramUniforms));
  const uint32_t jointHistogramPartialsSize = (m_num_bins * m_num_bins) * m_joint_histogram_dispatch_grid.workgroup_count();
  m_joint_histogram_kernel = m_compute_context->new_kernel({
      .compute_shader =
          {
              .shader_source = ShaderFile{"shaders/registration/joint_histogram.slang"},
              .entryPoint = "rawHistogram",
              .workgroup_size = joint_histogram_wg_size,
              .constants = {{"kNumBins", m_num_bins},
                            {"kUseFixedMask", static_cast<uint32_t>(m_use_fixed_mask)},
                            {"kUseMovingMask", static_cast<uint32_t>(m_use_moving_mask)}},
          },
      .bindings_map = {{"uniforms", m_joint_histogram_uniforms_buffer},
                       {"fixedTexture", m_fixed},
                       {"movingTexture", m_moving},
                       {"fixedMaskTexture", m_fixed_mask},
                       {"movingMaskTexture", m_moving_mask},
                       {"jointHistogram", m_raw_joint_histogram_buffer},
                       {"sampler", m_compute_context->new_linear_sampler()}},
  });

  m_compute_total_mass_kernel = m_compute_context->new_kernel({
      .compute_shader =
          {
              .shader_source = ShaderFile{"shaders/registration/joint_histogram.slang"},
              .entryPoint = "computeTotalMass",
              .constants = {{"kNumBins", m_num_bins},
                            {"kUseFixedMask", static_cast<uint32_t>(m_use_fixed_mask)},
                            {"kUseMovingMask", static_cast<uint32_t>(m_use_moving_mask)}},
          },
      .bindings_map = {{"jointHistogramSmoothed", m_smoothed_joint_histogram_buffer},
                       {"jointHistogramMass", m_joint_histogram_mass_buffer}},
  });

  m_joint_histogram_smooth_kernel = m_compute_context->new_kernel({
      .compute_shader =
          {
              .shader_source = ShaderFile{"shaders/registration/joint_histogram.slang"},
              .entryPoint = "smoothHistogram",
              .workgroup_size = WorkgroupSize{8, 8, 1},
              .constants = {{"kNumBins", m_num_bins},
                            {"kUseFixedMask", static_cast<uint32_t>(m_use_fixed_mask)},
                            {"kUseMovingMask", static_cast<uint32_t>(m_use_moving_mask)}},
          },
      .bindings_map = {{"uniforms", m_joint_histogram_uniforms_buffer},
                       {"jointHistogram", m_raw_joint_histogram_buffer},
                       {"jointHistogramSmoothed", m_smoothed_joint_histogram_buffer}},
  });

  m_precompute_kernel = m_compute_context->new_kernel({
      .compute_shader = {.shader_source = ShaderFile{"shaders/registration/nmi.slang"},
                         .entryPoint = "precompute",
                         .constants = {{"kNumBins", m_num_bins},
                                       {"kUseTargetMask", static_cast<uint32_t>(m_use_fixed_mask)},
                                       {"kUseSourceMask", static_cast<uint32_t>(m_use_moving_mask)}}},
      .bindings_map = {{"jointHistogram", m_smoothed_joint_histogram_buffer},
                       {"jointHistogramMass", m_joint_histogram_mass_buffer},
                       {"coefficientsTable", m_precomputed_coefficients_buffer},
                       {"mutualInformation", m_mutual_information_buffer}},
  });

  if (m_output == CalculatorOutput::CostAndGradients) {
    m_gradients_kernel = m_compute_context->new_kernel({
        .compute_shader =
            {
                .shader_source = ShaderFile{"shaders/registration/nmi.slang"},
                .entryPoint = "main",
                .workgroup_size = gradientsWorkgroupSize,
                .constants = {{"kNumBins", m_num_bins},
                              {"kUseTargetMask", static_cast<uint32_t>(m_use_fixed_mask)},
                              {"kUseSourceMask", static_cast<uint32_t>(m_use_moving_mask)}},
                .entry_point_args = {is_affine ? "AffineTransformation" : "RigidTransformation"},
            },
        .bindings_map = {{"uniforms", m_gradients_uniforms_buffer},
                         {"targetTexture", m_fixed},
                         {"sourceTexture", m_moving},
                         {"targetMaskTexture", m_fixed_mask},
                         {"sourceMaskTexture", m_moving_mask},
                         {"coefficientsTable", m_precomputed_coefficients_buffer},
                         {"partialSumsGradients", m_gradients_buffer},
                         {"sampler", m_compute_context->new_linear_sampler()}},
    });
  }
}

void NMICalculator::update(const GlobalTransform &transformation) {
  m_compute_context->clear_buffer(m_raw_joint_histogram_buffer);
  m_compute_context->clear_buffer(m_joint_histogram_mass_buffer);

  assert(transformation.param_count() == m_degrees_of_freedom);
  const auto moving_dispatch_grid = DispatchGrid::element_wise_texture(m_moving, m_min_max_moving_kernel.workgroup_size);
  const auto fixed_dispatch_grid = DispatchGrid::element_wise_texture(m_fixed, m_joint_histogram_kernel.workgroup_size);

  const auto transformation_matrix = transformation.to_matrix4f();

  const Eigen::Matrix4f transformation_matrix_voxel_space =
      Eigen::Map<const Eigen::Matrix4f>(m_voxel_scanner_matrices.scanner_to_voxel_moving.data()) *
      Eigen::Map<const Eigen::Matrix4f>(transformation_matrix.data()) *
      Eigen::Map<const Eigen::Matrix4f>(m_voxel_scanner_matrices.voxel_to_scanner_fixed.data());

  const JointHistogramUniforms joint_histogram_uniforms{
      .dispatch_grid = fixed_dispatch_grid,
      .intensities = m_intensities,
      .transformation_matrix = EigenHelpers::to_array(transformation_matrix_voxel_space),
  };
  m_compute_context->write_to_buffer(
      m_joint_histogram_uniforms_buffer, &joint_histogram_uniforms, sizeof(joint_histogram_uniforms));
  m_compute_context->dispatch_kernel(m_joint_histogram_kernel, m_joint_histogram_dispatch_grid);

  const WorkgroupSize smoothWGSize{8, 8, 1};
  const DispatchGrid smooth_grid =
      DispatchGrid::element_wise({size_t(m_num_bins), size_t(m_num_bins), size_t(1)}, smoothWGSize);
  m_compute_context->dispatch_kernel(m_joint_histogram_smooth_kernel, smooth_grid);

  const uint32_t histogramSize = m_num_bins * m_num_bins;
  const DispatchGrid merge_grid{.x = histogramSize};
  m_compute_context->dispatch_kernel(m_compute_total_mass_kernel, DispatchGrid{1, 1, 1});

  // Precompute coefficients and mutual information from the smoothed histogram
  m_compute_context->dispatch_kernel(m_precompute_kernel, DispatchGrid{1, 1, 1});

  const std::array<float, 3> pivot_array = EigenHelpers::to_array(transformation.pivot());

  if (m_output == CalculatorOutput::CostAndGradients) {
    if (transformation.is_affine()) {
      std::array<float, 12> params;
      const auto current = transformation.parameters();
      std::copy_n(current.begin(), 12, params.begin());
      const AffineGradientsUniforms gradients_uniforms{
          .dispatch_grid = m_gradients_dispatch_grid,
          .transformation_pivot = pivot_array,
          .intensities = m_intensities,
          .current_transform = params,
          .voxel_scanner_matrices = m_voxel_scanner_matrices,
      };

      m_compute_context->write_to_buffer(m_gradients_uniforms_buffer, &gradients_uniforms, sizeof(AffineGradientsUniforms));
    } else {
      std::array<float, 6> params;
      const auto current = transformation.parameters();
      std::copy_n(current.begin(), 6, params.begin());
      const RigidGradientsUniforms gradients_uniforms{
          .dispatch_grid = m_gradients_dispatch_grid,
          .transformation_pivot = pivot_array,
          .intensities = m_intensities,
          .current_transform = params,
          .voxel_scanner_matrices = m_voxel_scanner_matrices,
      };
      m_compute_context->write_to_buffer(m_gradients_uniforms_buffer, &gradients_uniforms, sizeof(RigidGradientsUniforms));
    }

    m_compute_context->dispatch_kernel(m_gradients_kernel, m_gradients_dispatch_grid);
  }
}

IterationResult NMICalculator::get_result() const {
  // Negate the cost and gradients since the class is used to maximize the mutual information
  // while the optimisation framework minimises the cost function.
  const auto mi_cost = m_compute_context->download_buffer_as_vector(m_mutual_information_buffer);
  if (m_output == CalculatorOutput::Cost) {
    return IterationResult{-mi_cost[0], {}};
  }

  const auto gradients_partials_f = m_compute_context->download_buffer_as_vector(m_gradients_buffer);
  const std::vector<double> gradients_partials(gradients_partials_f.begin(), gradients_partials_f.end());
  auto gradients = Utils::chunkReduce(gradients_partials, m_degrees_of_freedom, std::plus<>{});
  std::transform(gradients.begin(), gradients.end(), gradients.begin(), std::negate<>{});

  return IterationResult{-mi_cost[0], std::vector<float>(gradients.begin(), gradients.end())};
}
} // namespace MR::GPU
