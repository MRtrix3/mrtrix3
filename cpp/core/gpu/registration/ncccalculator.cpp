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

#include "gpu/registration/ncccalculator.h"

#include "gpu/registration/calculatoroutput.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/gpu.h"
#include "gpu/registration/registrationtypes.h"
#include "gpu/registration/voxelscannermatrices.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace MR::GPU {
namespace {

constexpr WorkgroupSize ncc_workgroup_size{8U, 4U, 4U};
constexpr double kVarianceEps = 1e-8;
constexpr double kDenominatorEps = 1e-8;

template <size_t N> struct alignas(16) NCCUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
  alignas(16) std::array<float, 3> transformation_pivot{};
  alignas(16) std::array<float, N> current_transform{};
  alignas(16) VoxelScannerMatrices voxel_scanner_matrices{};
};

using RigidNCCUniforms = NCCUniforms<6>;
using AffineNCCUniforms = NCCUniforms<12>;
static_assert(sizeof(RigidNCCUniforms) % 16 == 0, "RigidNCCUniforms must be 16-byte aligned");
static_assert(sizeof(AffineNCCUniforms) % 16 == 0, "AffineNCCUniforms must be 16-byte aligned");

template <size_t N>
void upload_uniforms(const ComputeContext &context,
                     const Buffer<std::byte> &buffer,
                     const DispatchGrid &dispatch_grid,
                     const GlobalTransform &transform,
                     const VoxelScannerMatrices &matrices) {
  NCCUniforms<N> uniforms{};
  uniforms.dispatch_grid = dispatch_grid;
  uniforms.transformation_pivot = EigenHelpers::to_array(transform.pivot());
  const auto params = transform.parameters();
  std::copy_n(params.begin(), N, uniforms.current_transform.begin());
  uniforms.voxel_scanner_matrices = matrices;
  context.write_to_buffer(buffer, &uniforms, sizeof(uniforms));
}

} // namespace

NCCCalculator::NCCCalculator(const Config &config)
    : m_output(config.output),
      m_compute_context(config.context),
      m_use_local_window(config.window_radius > 0U),
      m_window_radius(config.window_radius),
      m_voxel_scanner_matrices(config.voxel_scanner_matrices),
      m_fixed(config.fixed),
      m_moving(config.moving),
      m_fixed_mask(config.fixed_mask.value_or(config.fixed)),
      m_moving_mask(config.moving_mask.value_or(config.moving)),
      m_use_fixed_mask(config.fixed_mask.has_value()),
      m_use_moving_mask(config.moving_mask.has_value()) {
  assert(m_compute_context != nullptr);
  const bool is_rigid = config.transformation_type == TransformationType::Rigid;
  m_degrees_of_freedom = is_rigid ? 6U : 12U;
  m_dispatch_grid = DispatchGrid::element_wise_texture(m_fixed, ncc_workgroup_size);
  m_terms_per_workgroup = 1U + m_degrees_of_freedom;
  m_global_terms_per_workgroup = 5U + 3U * m_degrees_of_freedom;

  const size_t uniformsSize = is_rigid ? sizeof(RigidNCCUniforms) : sizeof(AffineNCCUniforms);
  m_uniforms_buffer = m_compute_context->new_empty_buffer<std::byte>(uniformsSize, BufferType::UniformBuffer);
  m_num_contributing_voxels_buffer = m_compute_context->new_empty_buffer<uint32_t>(1);

  if (m_use_local_window) {
    m_lncc_partials_buffer =
        m_compute_context->new_empty_buffer<float>(m_terms_per_workgroup * m_dispatch_grid.workgroup_count());
    m_lncc_kernel = m_compute_context->new_kernel({
        .compute_shader =
            {
                .shader_source = ShaderFile{"shaders/registration/ncc.slang"},
                .entryPoint = "lncc_main",
                .workgroup_size = ncc_workgroup_size,
                .constants = {{"kUseSourceMask", static_cast<uint32_t>(m_use_moving_mask)},
                              {"kUseTargetMask", static_cast<uint32_t>(m_use_fixed_mask)},
                              {"kComputeGradients",
                               static_cast<uint32_t>(m_output == CalculatorOutput::CostAndGradients)},
                              {"kWindowRadius", m_window_radius}},
                .entry_point_args = {is_rigid ? "RigidTransformation" : "AffineTransformation"},
            },
        .bindings_map = {{"uniforms", m_uniforms_buffer},
                         {"sourceImage", m_moving},
                         {"targetImage", m_fixed},
                         {"sourceMask", m_moving_mask},
                         {"targetMask", m_fixed_mask},
                         {"linearSampler", m_compute_context->new_linear_sampler()},
                         {"lnccPartials", m_lncc_partials_buffer},
                         {"numContributingVoxels", m_num_contributing_voxels_buffer}},
    });
  } else {
    m_global_partials_buffer =
        m_compute_context->new_empty_buffer<float>(m_global_terms_per_workgroup * m_dispatch_grid.workgroup_count());
    m_global_kernel = m_compute_context->new_kernel({
        .compute_shader =
            {
                .shader_source = ShaderFile{"shaders/registration/ncc.slang"},
                .entryPoint = "global_ncc_main",
                .workgroup_size = ncc_workgroup_size,
                .constants = {{"kUseSourceMask", static_cast<uint32_t>(m_use_moving_mask)},
                              {"kUseTargetMask", static_cast<uint32_t>(m_use_fixed_mask)},
                              {"kComputeGradients",
                               static_cast<uint32_t>(m_output == CalculatorOutput::CostAndGradients)},
                              {"kWindowRadius", m_window_radius}},
                .entry_point_args = {is_rigid ? "RigidTransformation" : "AffineTransformation"},
            },
        .bindings_map = {{"uniforms", m_uniforms_buffer},
                         {"sourceImage", m_moving},
                         {"targetImage", m_fixed},
                         {"sourceMask", m_moving_mask},
                         {"targetMask", m_fixed_mask},
                         {"linearSampler", m_compute_context->new_linear_sampler()},
                         {"globalPartials", m_global_partials_buffer},
                         {"numContributingVoxels", m_num_contributing_voxels_buffer}},
    });
  }
}

void NCCCalculator::update(const GlobalTransform &transformation) {
  assert(transformation.param_count() == m_degrees_of_freedom);
  if (m_use_local_window) {
    assert(m_window_radius > 0U);
  }

  if (transformation.is_affine()) {
    upload_uniforms<12>(*m_compute_context, m_uniforms_buffer, m_dispatch_grid, transformation, m_voxel_scanner_matrices);
  } else {
    upload_uniforms<6>(*m_compute_context, m_uniforms_buffer, m_dispatch_grid, transformation, m_voxel_scanner_matrices);
  }

  m_compute_context->clear_buffer(m_num_contributing_voxels_buffer);

  if (m_use_local_window) {
    m_compute_context->dispatch_kernel(m_lncc_kernel, m_dispatch_grid);
  } else {
    m_compute_context->dispatch_kernel(m_global_kernel, m_dispatch_grid);
  }
}

IterationResult NCCCalculator::get_result() const {
  if (m_use_local_window) {
    const auto partials = m_compute_context->download_buffer_as_vector(m_lncc_partials_buffer);
    const auto contributing = m_compute_context->download_buffer_as_vector(m_num_contributing_voxels_buffer);
    const uint32_t validCount = contributing.empty() ? 0U : contributing[0];
    const size_t workgroups = m_dispatch_grid.workgroup_count();

    double totalCost = 0.0;
    std::vector<double> gradients;
    if (m_output == CalculatorOutput::CostAndGradients) {
      gradients.assign(m_degrees_of_freedom, 0.0);
    }

    for (size_t wg = 0; wg < workgroups; ++wg) {
      const size_t base = wg * m_terms_per_workgroup;
      totalCost += partials[base];
      if (m_output == CalculatorOutput::CostAndGradients) {
        for (uint32_t i = 0; i < m_degrees_of_freedom; ++i) {
          gradients[i] += partials[base + 1 + i];
        }
      }
    }

    const float invCount = validCount > 0U ? 1.0F / static_cast<float>(validCount) : 0.0F;
    const float cost = static_cast<float>(totalCost) * invCount;

    if (m_output == CalculatorOutput::Cost) {
      return IterationResult{cost, {}};
    }

    std::vector<float> gradientsF(m_degrees_of_freedom, 0.0F);
    for (uint32_t i = 0; i < m_degrees_of_freedom; ++i) {
      gradientsF[i] = static_cast<float>(gradients[i]) * invCount;
    }
    return IterationResult{cost, std::move(gradientsF)};
  }

  const auto partials = m_compute_context->download_buffer_as_vector(m_global_partials_buffer);
  const auto contributing = m_compute_context->download_buffer_as_vector(m_num_contributing_voxels_buffer);
  const double validCount = contributing.empty() ? 0.0 : static_cast<double>(contributing[0]);
  if (validCount == 0.0) {
    return m_output == CalculatorOutput::CostAndGradients
               ? IterationResult{0.0F, std::vector<float>(m_degrees_of_freedom, 0.0F)}
               : IterationResult{0.0F, {}};
  }

  const size_t workgroups = m_dispatch_grid.workgroup_count();
  double sumTarget = 0.0;
  double sumMoving = 0.0;
  double sumTargetSquared = 0.0;
  double sumMovingSquared = 0.0;
  double sumTargetMoving = 0.0;
  std::vector<double> sumTargetMovingPrime(m_degrees_of_freedom, 0.0);
  std::vector<double> sumMovingPrime(m_degrees_of_freedom, 0.0);
  std::vector<double> sumMovingSquaredPrime(m_degrees_of_freedom, 0.0);

  for (size_t wg = 0; wg < workgroups; ++wg) {
    const size_t base = wg * m_global_terms_per_workgroup;
    size_t offset = base;
    sumTarget += partials[offset++];
    sumMoving += partials[offset++];
    sumTargetSquared += partials[offset++];
    sumMovingSquared += partials[offset++];
    sumTargetMoving += partials[offset++];

    for (uint32_t i = 0; i < m_degrees_of_freedom; ++i) {
      sumTargetMovingPrime[i] += partials[offset++];
    }
    for (uint32_t i = 0; i < m_degrees_of_freedom; ++i) {
      sumMovingPrime[i] += partials[offset++];
    }
    for (uint32_t i = 0; i < m_degrees_of_freedom; ++i) {
      sumMovingSquaredPrime[i] += partials[offset++];
    }
  }

  const double invCount = 1.0 / validCount;
  const double meanTarget = sumTarget * invCount;
  const double meanMoving = sumMoving * invCount;
  const double varianceTarget = std::max(0.0, sumTargetSquared * invCount - meanTarget * meanTarget);
  const double varianceMoving = std::max(0.0, sumMovingSquared * invCount - meanMoving * meanMoving);
  if (varianceTarget < kVarianceEps || varianceMoving < kVarianceEps) {
    return m_output == CalculatorOutput::CostAndGradients
               ? IterationResult{0.0F, std::vector<float>(m_degrees_of_freedom, 0.0F)}
               : IterationResult{0.0F, {}};
  }

  const double covariance = sumTargetMoving * invCount - meanTarget * meanMoving;
  const double denom = std::sqrt(std::max(varianceTarget * varianceMoving, kVarianceEps));
  const float cost = static_cast<float>(-covariance / denom);

  if (m_output == CalculatorOutput::Cost) {
    return IterationResult{cost, {}};
  }

  const double denomGradBase = std::max(varianceMoving * denom, kDenominatorEps);
  std::vector<float> gradients(m_degrees_of_freedom, 0.0F);
  for (uint32_t i = 0; i < m_degrees_of_freedom; ++i) {
    const double cPrime = (sumTargetMovingPrime[i] * invCount) - (meanTarget * sumMovingPrime[i] * invCount);
    const double varMovingPrime =
        2.0 * (sumMovingSquaredPrime[i] * invCount - meanMoving * sumMovingPrime[i] * invCount);
    const double gradient = (cPrime * varianceMoving - 0.5 * covariance * varMovingPrime) / denomGradBase;
    gradients[i] = static_cast<float>(-gradient);
  }

  return IterationResult{cost, std::move(gradients)};
}

} // namespace MR::GPU
