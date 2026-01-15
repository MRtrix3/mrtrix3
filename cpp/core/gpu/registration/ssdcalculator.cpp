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

#include "gpu/registration/ssdcalculator.h"

#include "gpu/registration/calculatoroutput.h"
#include "gpu/registration/eigenhelpers.h"
#include "exception.h"
#include "gpu/gpu.h"
#include "gpu/registration/registrationtypes.h"
#include "gpu/registration/voxelscannermatrices.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace MR::GPU {

template <size_t N> struct SSDUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
  alignas(16) std::array<float, 3> transformationPivot{};
  alignas(16) std::array<float, N> currentTransform{};
  alignas(16) VoxelScannerMatrices voxelScannerMatrices{};
};

using RigidSSDUniforms = SSDUniforms<6>;
using AffineSSDUniforms = SSDUniforms<12>;
static_assert(sizeof(RigidSSDUniforms) % 16 == 0, "RigidSSDUniforms must be 16-byte aligned");
static_assert(sizeof(AffineSSDUniforms) % 16 == 0, "AffineSSDUniforms must be 16-byte aligned");

constexpr WorkgroupSize ssd_workgroup_size{8, 8, 4};

SSDCalculator::SSDCalculator(const Config &config)
    : m_output(config.output),
      m_compute_context(config.context),
      m_fixed(config.fixed),
      m_moving(config.moving),
      m_fixed_mask(config.fixed_mask.value_or(config.fixed)),
      m_moving_mask(config.moving_mask.value_or(config.moving)),
      m_use_fixed_mask(config.fixed_mask.has_value()),
      m_use_moving_mask(config.moving_mask.has_value()),
      m_voxel_scanner_matrices(config.voxel_scanner_matrices) {
  assert(m_compute_context != nullptr);
  const bool is_rigid = config.transformation_type == TransformationType::Rigid;
  m_degrees_of_freedom = is_rigid ? 6U : 12U;

  m_dispatch_grid = DispatchGrid::element_wise_texture(m_fixed, ssd_workgroup_size);

  const uint32_t uniforms_size = is_rigid ? sizeof(RigidSSDUniforms) : sizeof(AffineSSDUniforms);
  m_uniforms_buffer = m_compute_context->new_empty_buffer<std::byte>(uniforms_size, BufferType::UniformBuffer);

  const size_t params_per_workgroup = 1U + m_degrees_of_freedom;
  m_partials_buffer = m_compute_context->new_empty_buffer<float>(params_per_workgroup * m_dispatch_grid.workgroup_count());
  m_num_contributing_voxels_buffer = m_compute_context->new_empty_buffer<uint32_t>(1);

  m_kernel = m_compute_context->new_kernel({
      .compute_shader =
          {
              .shader_source = ShaderFile{"shaders/registration/ssd.slang"},
              .entryPoint = "main",
              .workgroup_size = ssd_workgroup_size,
              .constants = {{"kUseSourceMask", static_cast<uint32_t>(m_use_moving_mask)},
                            {"kUseTargetMask", static_cast<uint32_t>(m_use_fixed_mask)},
                            {"kComputeGradients",
                             static_cast<uint32_t>(m_output == CalculatorOutput::CostAndGradients)}},
              .entry_point_args = {is_rigid ? "RigidTransformation" : "AffineTransformation"},
          },
      .bindings_map = {{"uniforms", m_uniforms_buffer},
                       {"sourceImage", m_moving},
                       {"targetImage", m_fixed},
                       {"sourceMask", m_moving_mask},
                       {"targetMask", m_fixed_mask},
                       {"linearSampler", m_compute_context->new_linear_sampler()},
                       {"ssdAndGradientsPartials", m_partials_buffer},
                       {"numContributingVoxels", m_num_contributing_voxels_buffer}},
  });
}
void SSDCalculator::update(const GlobalTransform &transformation) {
  assert(transformation.param_count() == m_degrees_of_freedom);
  m_compute_context->clear_buffer(m_num_contributing_voxels_buffer);

  const std::array<float, 3> pivotArray = EigenHelpers::to_array(transformation.pivot());
  if (transformation.is_affine()) {
    std::array<float, 12> params;
    const auto current = transformation.parameters();
    std::copy_n(current.begin(), 12, params.begin());
    const AffineSSDUniforms uniforms{
        .dispatch_grid = m_dispatch_grid,
        .transformationPivot = pivotArray,
        .currentTransform = params,
        .voxelScannerMatrices = m_voxel_scanner_matrices,
    };
    m_compute_context->write_to_buffer(m_uniforms_buffer, &uniforms, sizeof(AffineSSDUniforms));
  } else {
    std::array<float, 6> params;
    const auto current = transformation.parameters();
    std::copy_n(current.begin(), 6, params.begin());
    const RigidSSDUniforms uniforms{
        .dispatch_grid = m_dispatch_grid,
        .transformationPivot = pivotArray,
        .currentTransform = params,
        .voxelScannerMatrices = m_voxel_scanner_matrices,
    };
    m_compute_context->write_to_buffer(m_uniforms_buffer, &uniforms, sizeof(RigidSSDUniforms));
  }

  m_compute_context->dispatch_kernel(m_kernel, m_dispatch_grid);
}

IterationResult SSDCalculator::get_result() const {
  const auto partials = m_compute_context->download_buffer_as_vector(m_partials_buffer);
  const size_t paramsPerWorkgroup = 1U + m_degrees_of_freedom;
  const size_t workgroups = m_dispatch_grid.workgroup_count();
  if (partials.size() < paramsPerWorkgroup * workgroups) {
    throw MR::Exception("SSDCalculator: partials buffer size mismatch.");
  }

  double cost = 0.0;
  const bool computeGradients = m_output == CalculatorOutput::CostAndGradients;
  std::vector<double> gradients;
  if (computeGradients) {
    gradients.assign(m_degrees_of_freedom, 0.0);
  }
  for (size_t wg = 0; wg < workgroups; ++wg) {
    const size_t base = wg * paramsPerWorkgroup;
    cost += partials[base];
    if (computeGradients) {
      for (size_t i = 0; i < m_degrees_of_freedom; ++i) {
        gradients[i] += partials[base + 1 + i];
      }
    }
  }

  if (!computeGradients) {
    return IterationResult{static_cast<float>(cost), {}};
  }

  std::vector<float> gradientsF;
  gradientsF.reserve(m_degrees_of_freedom);
  for (double value : gradients) {
    gradientsF.push_back(static_cast<float>(value));
  }

  return IterationResult{static_cast<float>(cost), std::move(gradientsF)};
}
} // namespace MR::GPU
