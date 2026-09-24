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

#include "gpu/registration/nonlinearssd.h"

#include "gpu/gpu.h"

#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace MR::GPU {
namespace {

constexpr WorkgroupSize workgroup_size{.x = 8U, .y = 4U, .z = 4U};

float evaluate_cost_from_partials(const ComputeContext &context,
                                  const Buffer<float> &cost_partials_buffer,
                                  const Buffer<float> &cost_counts_buffer) {
  const std::vector<float> partials = context.download_buffer_as_vector(cost_partials_buffer);
  const std::vector<float> counts = context.download_buffer_as_vector(cost_counts_buffer);
  const double total_cost = std::reduce(partials.begin(), partials.end(), 0.0);
  const double total_count = std::reduce(counts.begin(), counts.end(), 0.0);
  if (!(total_count > 0.0)) {
    return std::numeric_limits<float>::infinity();
  }
  return static_cast<float>(total_cost / total_count);
}

} // namespace

NonlinearSsdPipeline::CostPhaseResources::CostPhaseResources(const ComputeContext &context,
                                                             const NonlinearSsdPipelineConfig &config,
                                                             std::string_view cost_entry_point,
                                                             std::string_view displacement_binding_name,
                                                             const DispatchGrid &dispatch_grid,
                                                             const Buffer<std::byte> &dispatch_uniforms_buffer)
    : dispatch_grid(dispatch_grid),
      cost_partials_buffer(context.new_empty_buffer<float>(dispatch_grid.workgroup_count())),
      cost_counts_buffer(context.new_empty_buffer<float>(dispatch_grid.workgroup_count())),
      cost_kernel(context.new_kernel(
          {.compute_shader =
               ShaderEntry{
                   .shader_source = ShaderFile{"shaders/registration/nonlinear/ssd_cost.slang"},
                   .entryPoint = std::string(cost_entry_point),
                   .workgroup_size = workgroup_size,
                   .constants = {{"kIncludeOutOfBounds", uint32_t{1}},
                                 {"kUseFixedMask", static_cast<uint32_t>(config.fixed_mask.has_value())},
                                 {"kUseMovingMask", static_cast<uint32_t>(config.moving_mask.has_value())}},
               },
           .bindings_map = {
               {"uniforms", dispatch_uniforms_buffer},
               {"fixedImage", config.fixed_image},
               {"movingImage", config.moving_image},
               {"fixedMask", config.fixed_mask.value_or(config.fixed_image)},
               {"movingMask", config.moving_mask.value_or(config.moving_image)},
               {std::string(displacement_binding_name), config.displacement},
               {"voxelScannerMatrices", config.voxel_scanner_buffer},
               {"linearSampler", config.linear_sampler},
               {"ssdPartials", cost_partials_buffer},
               {"ssdCounts", cost_counts_buffer},
           }})) {}

NonlinearSsdPipeline::UpdatePhaseResources::UpdatePhaseResources(const ComputeContext &context,
                                                                 const NonlinearSsdPipelineConfig &config,
                                                                 std::string_view update_entry_point,
                                                                 std::string_view displacement_binding_name,
                                                                 const DispatchGrid &dispatch_grid,
                                                                 const Texture &output_update)
    : dispatch_grid(dispatch_grid),
      update_kernel(context.new_kernel({
          .compute_shader =
              ShaderEntry{
                  .shader_source = ShaderFile{"shaders/registration/nonlinear/local_ssd_update.slang"},
                  .entryPoint = std::string(update_entry_point),
                  .workgroup_size = workgroup_size,
                  .constants = {{"kAlpha", config.alpha},
                                {"kEpsilon", config.epsilon},
                                {"kMaxUpdateMagnitude", config.max_update_magnitude},
                                {"kUseFixedMask", static_cast<uint32_t>(config.fixed_mask.has_value())},
                                {"kUseMovingMask", static_cast<uint32_t>(config.moving_mask.has_value())}},
              },
          .bindings_map =
              {
                  {"fixedImage", config.fixed_image},
                  {"movingImage", config.moving_image},
                  {"fixedMask", config.fixed_mask.value_or(config.fixed_image)},
                  {"movingMask", config.moving_mask.value_or(config.moving_image)},
                  {std::string(displacement_binding_name), config.displacement},
                  {"voxelScannerMatrices", config.voxel_scanner_buffer},
                  {"outputUpdate", output_update},
                  {"linearSampler", config.linear_sampler},
              },
      })) {}

float NonlinearSsdPipeline::evaluate_cost_phase(const ComputeContext &context, const CostPhaseResources &phase) {
  context.dispatch_kernel(phase.cost_kernel, phase.dispatch_grid);
  return evaluate_cost_from_partials(context, phase.cost_partials_buffer, phase.cost_counts_buffer);
}

void NonlinearSsdPipeline::dispatch_update_phase(const ComputeContext &context, const UpdatePhaseResources &phase) {
  context.dispatch_kernel(phase.update_kernel, phase.dispatch_grid);
}

NonlinearSsdPipeline::NonlinearSsdPipeline(const ComputeContext &context, const NonlinearSsdPipelineConfig &config)
    : m_forward_cost_phase(context,
                           config,
                           "ssd_cost_forward",
                           "displacement",
                           config.forward_dispatch_grid,
                           config.forward_dispatch_uniforms_buffer),
      m_backward_cost_phase(context,
                            config,
                            "ssd_cost_backward",
                            "displacementBackward",
                            config.backward_dispatch_grid,
                            config.backward_dispatch_uniforms_buffer),
      m_forward_update_phase(context,
                             config,
                             "ssd_update_forward",
                             "displacement",
                             config.forward_dispatch_grid,
                             config.forward_update_output),
      m_backward_update_phase(context,
                              config,
                              "ssd_update_backward",
                              "displacementBackward",
                              config.backward_dispatch_grid,
                              config.backward_update_output) {}

float NonlinearSsdPipeline::evaluate_forward_cost(const ComputeContext &context) const {
  return evaluate_cost_phase(context, m_forward_cost_phase);
}

float NonlinearSsdPipeline::evaluate_backward_cost(const ComputeContext &context) const {
  return evaluate_cost_phase(context, m_backward_cost_phase);
}

void NonlinearSsdPipeline::compute_forward_update(const ComputeContext &context) const {
  dispatch_update_phase(context, m_forward_update_phase);
}

void NonlinearSsdPipeline::compute_backward_update(const ComputeContext &context) const {
  dispatch_update_phase(context, m_backward_update_phase);
}

} // namespace MR::GPU
