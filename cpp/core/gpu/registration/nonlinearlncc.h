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

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace MR::GPU {

struct NonlinearLnccPipelineConfig {
  uint32_t window_radius = 1U;
  float sigma_ratio = 0.0F;
  float moment_epsilon = 0.0F;
  float rho_epsilon = 0.0F;
  float denominator_epsilon = 0.0F;
  float max_update_magnitude = 0.0F;

  TextureSpec fixed_moments_texture_spec{};
  TextureSpec moving_moments_texture_spec{};

  Texture fixed_image{};
  Texture moving_image{};
  Texture displacement{};
  std::optional<Texture> fixed_mask;
  std::optional<Texture> moving_mask;

  Buffer<std::byte> forward_dispatch_uniforms_buffer{};
  Buffer<std::byte> backward_dispatch_uniforms_buffer{};
  Buffer<std::byte> voxel_scanner_buffer{};

  Sampler linear_sampler{};

  DispatchGrid forward_dispatch_grid{};
  DispatchGrid backward_dispatch_grid{};

  Texture forward_update_output{};
  Texture backward_update_output{};
};

class NonlinearLnccPipeline {
public:
  NonlinearLnccPipeline(const ComputeContext &context, const NonlinearLnccPipelineConfig &config);
  ~NonlinearLnccPipeline() = default;

  NonlinearLnccPipeline(const NonlinearLnccPipeline &) = delete;
  NonlinearLnccPipeline &operator=(const NonlinearLnccPipeline &) = delete;
  NonlinearLnccPipeline(NonlinearLnccPipeline &&) noexcept = default;
  NonlinearLnccPipeline &operator=(NonlinearLnccPipeline &&) noexcept = default;

  float evaluate_forward_cost(const ComputeContext &context) const;
  float evaluate_backward_cost(const ComputeContext &context) const;

  void compute_forward_update(const ComputeContext &context) const;
  void compute_backward_update(const ComputeContext &context) const;

private:
  struct UpdateScratchTextures;

  // Resources for LNCC cost evaluation
  struct CostPhaseResources {
    CostPhaseResources(const ComputeContext &context,
                       uint32_t shader_window_radius,
                       float moment_epsilon,
                       const UpdateScratchTextures &scratch,
                       std::string_view prepare_entry_point,
                       std::string_view reduce_entry_point,
                       std::string_view reduce_lattice_binding_name,
                       const Texture &lattice_image,
                       const DispatchGrid &dispatch_grid,
                       const Buffer<std::byte> &dispatch_uniforms_buffer,
                       const Texture &fixed_image,
                       const Texture &moving_image,
                       const Texture &fixed_mask,
                       const Texture &moving_mask,
                       bool use_fixed_mask,
                       bool use_moving_mask,
                       const Texture &displacement,
                       const Buffer<std::byte> &voxel_scanner_buffer,
                       const Sampler &linear_sampler);

    uint32_t window_radius = 0U;
    uint32_t lattice_width = 0U;
    uint32_t lattice_height = 0U;
    uint32_t lattice_depth = 0U;
    uint32_t slab_depth = 0U;
    Buffer<std::byte> slab_uniforms_buffer;
    DispatchGrid prepare_dispatch_grid;
    DispatchGrid filter_x_dispatch_grid;
    DispatchGrid filter_y_dispatch_grid;
    Buffer<float> cost_partials_buffer;
    Buffer<float> cost_counts_buffer;
    Kernel prepare_raw_moments_kernel;
    Kernel filter_x_kernel;
    Kernel filter_y_kernel;
    Kernel reduce_z_kernel;
  };

  struct UpdateScratchTextures {
    UpdateScratchTextures(const ComputeContext &context, const NonlinearLnccPipelineConfig &config);

    // We use a shared scrach pool made of RGBAF32 textures for both cost and update passes.
    // A ping-pong strategy is used for the separable box filtering steps.
    // LNCC cost only consumes moments 1-2 (gradient channels ignored);
    // LNCC update consumes moments 1-4 for the update solve.
    Texture moments_ping_1;
    Texture moments_ping_2;
    Texture moments_ping_3;
    Texture moments_ping_4;
    Texture moments_pong_1;
    Texture moments_pong_2;
    Texture moments_pong_3;
    Texture moments_pong_4;
  };

  // Resources for LNCC local update computation
  struct UpdatePhaseResources {
    UpdatePhaseResources(const ComputeContext &context,
                         const NonlinearLnccPipelineConfig &config,
                         const UpdateScratchTextures &scratch,
                         std::string_view prepare_entry_point,
                         std::string_view reduce_entry_point,
                         const Texture &lattice_image,
                         const Texture &output_update);

    uint32_t window_radius = 0U;
    uint32_t lattice_width = 0U;
    uint32_t lattice_height = 0U;
    uint32_t lattice_depth = 0U;
    uint32_t slab_depth = 0U;
    Buffer<std::byte> slab_uniforms_buffer;
    DispatchGrid prepare_dispatch_grid;
    DispatchGrid filter_x_dispatch_grid;
    DispatchGrid filter_y_dispatch_grid;
    Texture moments_ping_1;
    Texture moments_ping_2;
    Texture moments_ping_3;
    Texture moments_ping_4;
    Texture moments_pong_1;
    Texture moments_pong_2;
    Texture moments_pong_3;
    Texture moments_pong_4;
    Kernel prepare_raw_moments_kernel;
    Kernel filter_x_kernel;
    Kernel filter_y_kernel;
    Kernel reduce_z_kernel;
  };

  static float evaluate_cost_phase(const ComputeContext &context, const CostPhaseResources &phase);
  static void dispatch_update_phase(const ComputeContext &context, const UpdatePhaseResources &phase);

  UpdateScratchTextures m_update_scratch;
  CostPhaseResources m_forward_cost_phase;
  CostPhaseResources m_backward_cost_phase;
  UpdatePhaseResources m_forward_update_phase;
  UpdatePhaseResources m_backward_update_phase;
};

} // namespace MR::GPU
