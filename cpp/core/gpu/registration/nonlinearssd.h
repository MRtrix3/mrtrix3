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

struct NonlinearSsdPipelineConfig {
  float alpha = 0.0F;
  float epsilon = 0.0F;
  float max_update_magnitude = 0.0F;

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

class NonlinearSsdPipeline {
public:
  NonlinearSsdPipeline(const ComputeContext &context, const NonlinearSsdPipelineConfig &config);
  ~NonlinearSsdPipeline() = default;

  NonlinearSsdPipeline(const NonlinearSsdPipeline &) = delete;
  NonlinearSsdPipeline &operator=(const NonlinearSsdPipeline &) = delete;
  NonlinearSsdPipeline(NonlinearSsdPipeline &&) noexcept = default;
  NonlinearSsdPipeline &operator=(NonlinearSsdPipeline &&) noexcept = default;

  float evaluate_forward_cost(const ComputeContext &context) const;
  float evaluate_backward_cost(const ComputeContext &context) const;

  void compute_forward_update(const ComputeContext &context) const;
  void compute_backward_update(const ComputeContext &context) const;

private:
  struct CostPhaseResources {
    CostPhaseResources(const ComputeContext &context,
                       const NonlinearSsdPipelineConfig &config,
                       std::string_view cost_entry_point,
                       std::string_view displacement_binding_name,
                       const DispatchGrid &dispatch_grid,
                       const Buffer<std::byte> &dispatch_uniforms_buffer);

    DispatchGrid dispatch_grid;
    Buffer<float> cost_partials_buffer;
    Buffer<float> cost_counts_buffer;
    Kernel cost_kernel;
  };

  struct UpdatePhaseResources {
    UpdatePhaseResources(const ComputeContext &context,
                         const NonlinearSsdPipelineConfig &config,
                         std::string_view update_entry_point,
                         std::string_view displacement_binding_name,
                         const DispatchGrid &dispatch_grid,
                         const Texture &output_update);

    DispatchGrid dispatch_grid;
    Kernel update_kernel;
  };

  static float evaluate_cost_phase(const ComputeContext &context, const CostPhaseResources &phase);
  static void dispatch_update_phase(const ComputeContext &context, const UpdatePhaseResources &phase);

  CostPhaseResources m_forward_cost_phase;
  CostPhaseResources m_backward_cost_phase;
  UpdatePhaseResources m_forward_update_phase;
  UpdatePhaseResources m_backward_update_phase;
};

} // namespace MR::GPU
