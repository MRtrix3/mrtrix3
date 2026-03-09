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

#include "gpu/registration/nonlinearlncc.h"
#include "gpu/gpu.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace MR::GPU {
namespace {

constexpr WorkgroupSize workgroup_size{.x = 8U, .y = 4U, .z = 4U};

ShaderEntry::ShaderConstantMap lncc_update_constants(const NonlinearLnccPipelineConfig &config) {
  return {{"kWindowRadius", config.window_radius},
          {"kSigmaRatio", config.sigma_ratio},
          {"kMomentEpsilon", config.moment_epsilon},
          {"kRhoEpsilon", config.rho_epsilon},
          {"kDenominatorEpsilon", config.denominator_epsilon},
          {"kMaxUpdateMagnitude", config.max_update_magnitude}};
}

TextureSpec make_update_scratch_texture_spec(const NonlinearLnccPipelineConfig &config) {
  // Allocate scratch to the larger lattice once and reuse it everywhere.
  // This is cheaper than maintaining per-direction pools because forward/backward
  // and cost/update kernels are dispatched sequentially.
  return TextureSpec{
      .width = std::max(config.fixed_moments_texture_spec.width, config.moving_moments_texture_spec.width),
      .height = std::max(config.fixed_moments_texture_spec.height, config.moving_moments_texture_spec.height),
      .depth = std::max(config.fixed_moments_texture_spec.depth, config.moving_moments_texture_spec.depth),
      .format = config.fixed_moments_texture_spec.format,
      .usage = {.storage_binding = true, .render_target = false},
  };
}

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

NonlinearLnccPipeline::CostPhaseResources::CostPhaseResources(const ComputeContext &context,
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
                                                              const Texture &displacement,
                                                              const Buffer<std::byte> &voxel_scanner_buffer,
                                                              const Sampler &linear_sampler)
    : prepare_dispatch_grid(dispatch_grid),
      reduce_dispatch_grid(dispatch_grid),
      cost_partials_buffer(context.new_empty_buffer<float>(dispatch_grid.workgroup_count())),
      cost_counts_buffer(context.new_empty_buffer<float>(dispatch_grid.workgroup_count())),
      prepare_raw_moments_kernel(context.new_kernel(
          {.compute_shader =
               ShaderEntry{
                   .shader_source = ShaderFile{"shaders/registration/nonlinear/lncc_cost.slang"},
                   .entryPoint = std::string(prepare_entry_point),
                   .workgroup_size = workgroup_size,
                   .constants = {{"kWindowRadius", shader_window_radius}, {"kMomentEpsilon", moment_epsilon}},
               },
           .bindings_map =
               {
                   {"fixedImage", fixed_image},
                   {"movingImage", moving_image},
                   {"displacement", displacement},
                   {"voxelScannerMatrices", voxel_scanner_buffer},
                   {"linearSampler", linear_sampler},
                   // Cost pass reuses the shared scratch pool.
                   {"outputMoments1", scratch.moments_ping_1},
                   {"outputMoments2", scratch.moments_ping_2},
               }})),
      filter_x_kernel(context.new_kernel(
          {.compute_shader =
               ShaderEntry{.shader_source = ShaderFile{"shaders/registration/nonlinear/lncc_cost.slang"},
                           .entryPoint = "lnccCostFilterXPass",
                           .constants = {{"kWindowRadius", shader_window_radius}, {"kMomentEpsilon", moment_epsilon}}},
           .bindings_map =
               {
                   {"latticeImage", lattice_image},
                   {"inputMoments1", scratch.moments_ping_1},
                   {"inputMoments2", scratch.moments_ping_2},
                   {"outputMoments1", scratch.moments_pong_1},
                   {"outputMoments2", scratch.moments_pong_2},
               }})),
      filter_y_kernel(context.new_kernel(
          {.compute_shader =
               ShaderEntry{.shader_source = ShaderFile{"shaders/registration/nonlinear/lncc_cost.slang"},
                           .entryPoint = "lnccCostFilterYPass",
                           .constants = {{"kWindowRadius", shader_window_radius}, {"kMomentEpsilon", moment_epsilon}}},
           .bindings_map =
               {
                   {"latticeImage", lattice_image},
                   {"inputMoments1", scratch.moments_pong_1},
                   {"inputMoments2", scratch.moments_pong_2},
                   {"outputMoments1", scratch.moments_ping_1},
                   {"outputMoments2", scratch.moments_ping_2},
               }})),
      reduce_z_kernel(context.new_kernel(
          {.compute_shader =
               ShaderEntry{.shader_source = ShaderFile{"shaders/registration/nonlinear/lncc_cost.slang"},
                           .entryPoint = std::string(reduce_entry_point),
                           .workgroup_size = workgroup_size,
                           .constants = {{"kWindowRadius", shader_window_radius}, {"kMomentEpsilon", moment_epsilon}}},
           .bindings_map = {
               {"uniforms", dispatch_uniforms_buffer},
               {std::string(reduce_lattice_binding_name), lattice_image},
               {"xyFilteredMoments1", scratch.moments_ping_1},
               {"xyFilteredMoments2", scratch.moments_ping_2},
               {"ssdPartials", cost_partials_buffer},
               {"ssdCounts", cost_counts_buffer},
           }})) {
  filter_x_dispatch_grid = DispatchGrid::element_wise_texture(lattice_image, filter_x_kernel.workgroup_size);
  filter_y_dispatch_grid = DispatchGrid::element_wise_texture(lattice_image, filter_y_kernel.workgroup_size);
}

NonlinearLnccPipeline::UpdateScratchTextures::UpdateScratchTextures(const ComputeContext &context,
                                                                    const NonlinearLnccPipelineConfig &config) {
  const TextureSpec scratch_spec = make_update_scratch_texture_spec(config);
  // 8 RGBA moment textures for staged X/Y filtering.
  // The same set is reused for both directions and for cost/update phases.
  moments_ping_1 = context.new_empty_texture(scratch_spec);
  moments_ping_2 = context.new_empty_texture(scratch_spec);
  moments_ping_3 = context.new_empty_texture(scratch_spec);
  moments_ping_4 = context.new_empty_texture(scratch_spec);
  moments_pong_1 = context.new_empty_texture(scratch_spec);
  moments_pong_2 = context.new_empty_texture(scratch_spec);
  moments_pong_3 = context.new_empty_texture(scratch_spec);
  moments_pong_4 = context.new_empty_texture(scratch_spec);
}

NonlinearLnccPipeline::UpdatePhaseResources::UpdatePhaseResources(const ComputeContext &context,
                                                                  const NonlinearLnccPipelineConfig &config,
                                                                  const UpdateScratchTextures &scratch,
                                                                  std::string_view prepare_entry_point,
                                                                  std::string_view reduce_entry_point,
                                                                  const Texture &lattice_image,
                                                                  const DispatchGrid &dispatch_grid,
                                                                  const Texture &output_update)
    : prepare_dispatch_grid(dispatch_grid),
      reduce_dispatch_grid(dispatch_grid),
      moments_ping_1(scratch.moments_ping_1),
      moments_ping_2(scratch.moments_ping_2),
      moments_ping_3(scratch.moments_ping_3),
      moments_ping_4(scratch.moments_ping_4),
      moments_pong_1(scratch.moments_pong_1),
      moments_pong_2(scratch.moments_pong_2),
      moments_pong_3(scratch.moments_pong_3),
      moments_pong_4(scratch.moments_pong_4) {
  const ShaderEntry prepare_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/local_lncc_update.slang"},
      .entryPoint = std::string(prepare_entry_point),
      .workgroup_size = workgroup_size,
      .constants = lncc_update_constants(config),
  };
  prepare_raw_moments_kernel = context.new_kernel({.compute_shader = prepare_shader,
                                                   .bindings_map = {
                                                       {"fixedImage", config.fixed_image},
                                                       {"movingImage", config.moving_image},
                                                       {"displacement", config.displacement},
                                                       {"voxelScannerMatrices", config.voxel_scanner_buffer},
                                                       {"linearSampler", config.linear_sampler},
                                                       {"outputMoments1", moments_ping_1},
                                                       {"outputMoments2", moments_ping_2},
                                                       {"outputMoments3", moments_ping_3},
                                                       {"outputMoments4", moments_ping_4},
                                                   }});

  const ShaderEntry filter_x_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/local_lncc_update.slang"},
      .entryPoint = "lnccUpdateFilterXPass",
      .constants = lncc_update_constants(config),
  };
  filter_x_kernel = context.new_kernel({.compute_shader = filter_x_shader,
                                        .bindings_map = {
                                            {"latticeImage", lattice_image},
                                            {"inputMoments1", moments_ping_1},
                                            {"inputMoments2", moments_ping_2},
                                            {"inputMoments3", moments_ping_3},
                                            {"inputMoments4", moments_ping_4},
                                            {"outputMoments1", moments_pong_1},
                                            {"outputMoments2", moments_pong_2},
                                            {"outputMoments3", moments_pong_3},
                                            {"outputMoments4", moments_pong_4},
                                        }});

  const ShaderEntry filter_y_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/local_lncc_update.slang"},
      .entryPoint = "lnccUpdateFilterYPass",
      .constants = lncc_update_constants(config),
  };
  filter_y_kernel = context.new_kernel({.compute_shader = filter_y_shader,
                                        .bindings_map = {
                                            {"latticeImage", lattice_image},
                                            {"inputMoments1", moments_pong_1},
                                            {"inputMoments2", moments_pong_2},
                                            {"inputMoments3", moments_pong_3},
                                            {"inputMoments4", moments_pong_4},
                                            {"outputMoments1", moments_ping_1},
                                            {"outputMoments2", moments_ping_2},
                                            {"outputMoments3", moments_ping_3},
                                            {"outputMoments4", moments_ping_4},
                                        }});

  const ShaderEntry reduce_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/local_lncc_update.slang"},
      .entryPoint = std::string(reduce_entry_point),
      .workgroup_size = workgroup_size,
      .constants = lncc_update_constants(config),
  };
  reduce_z_kernel = context.new_kernel({.compute_shader = reduce_shader,
                                        .bindings_map = {
                                            {"latticeImage", lattice_image},
                                            {"fixedImage", config.fixed_image},
                                            {"movingImage", config.moving_image},
                                            {"displacement", config.displacement},
                                            {"voxelScannerMatrices", config.voxel_scanner_buffer},
                                            {"linearSampler", config.linear_sampler},
                                            {"xyFilteredMoments1", moments_ping_1},
                                            {"xyFilteredMoments2", moments_ping_2},
                                            {"xyFilteredMoments3", moments_ping_3},
                                            {"xyFilteredMoments4", moments_ping_4},
                                            {"outputUpdate", output_update},
                                        }});
  filter_x_dispatch_grid = DispatchGrid::element_wise_texture(lattice_image, filter_x_kernel.workgroup_size);
  filter_y_dispatch_grid = DispatchGrid::element_wise_texture(lattice_image, filter_y_kernel.workgroup_size);
}

float NonlinearLnccPipeline::evaluate_cost_phase(const ComputeContext &context, const CostPhaseResources &phase) {
  context.dispatch_kernel(phase.prepare_raw_moments_kernel, phase.prepare_dispatch_grid);
  context.dispatch_kernel(phase.filter_x_kernel, phase.filter_x_dispatch_grid);
  context.dispatch_kernel(phase.filter_y_kernel, phase.filter_y_dispatch_grid);
  context.dispatch_kernel(phase.reduce_z_kernel, phase.reduce_dispatch_grid);
  return evaluate_cost_from_partials(context, phase.cost_partials_buffer, phase.cost_counts_buffer);
}

void NonlinearLnccPipeline::dispatch_update_phase(const ComputeContext &context, const UpdatePhaseResources &phase) {
  context.dispatch_kernel(phase.prepare_raw_moments_kernel, phase.prepare_dispatch_grid);
  context.dispatch_kernel(phase.filter_x_kernel, phase.filter_x_dispatch_grid);
  context.dispatch_kernel(phase.filter_y_kernel, phase.filter_y_dispatch_grid);
  context.dispatch_kernel(phase.reduce_z_kernel, phase.reduce_dispatch_grid);
}

NonlinearLnccPipeline::NonlinearLnccPipeline(const ComputeContext &context, const NonlinearLnccPipelineConfig &config)
    // `m_update_scratch` is the single LNCC intermediate pool. Every phase binds
    // this pool and relies on serialized dispatch order, reducing peak VRAM.
    : m_update_scratch(context, config),
      m_forward_cost_phase(context,
                           config.window_radius,
                           config.moment_epsilon,
                           m_update_scratch,
                           "lnccPrepareRawMomentsForward",
                           "lnccCostForwardReduceZ",
                           "fixedImage",
                           config.fixed_image,
                           config.forward_dispatch_grid,
                           config.forward_dispatch_uniforms_buffer,
                           config.fixed_image,
                           config.moving_image,
                           config.displacement,
                           config.voxel_scanner_buffer,
                           config.linear_sampler),
      m_backward_cost_phase(context,
                            config.window_radius,
                            config.moment_epsilon,
                            m_update_scratch,
                            "lnccPrepareRawMomentsBackward",
                            "lnccCostBackwardReduceZ",
                            "movingImage",
                            config.moving_image,
                            config.backward_dispatch_grid,
                            config.backward_dispatch_uniforms_buffer,
                            config.fixed_image,
                            config.moving_image,
                            config.displacement,
                            config.voxel_scanner_buffer,
                            config.linear_sampler),
      m_forward_update_phase(context,
                             config,
                             m_update_scratch,
                             "lnccUpdatePrepareRawMomentsForward",
                             "lnccUpdateForwardReduceZ",
                             config.fixed_image,
                             config.forward_dispatch_grid,
                             config.forward_update_output),
      m_backward_update_phase(context,
                              config,
                              m_update_scratch,
                              "lnccUpdatePrepareRawMomentsBackward",
                              "lnccUpdateBackwardReduceZ",
                              config.moving_image,
                              config.backward_dispatch_grid,
                              config.backward_update_output) {}

float NonlinearLnccPipeline::evaluate_forward_cost(const ComputeContext &context) const {
  return evaluate_cost_phase(context, m_forward_cost_phase);
}

float NonlinearLnccPipeline::evaluate_backward_cost(const ComputeContext &context) const {
  return evaluate_cost_phase(context, m_backward_cost_phase);
}

void NonlinearLnccPipeline::compute_forward_update(const ComputeContext &context) const {
  dispatch_update_phase(context, m_forward_update_phase);
}

void NonlinearLnccPipeline::compute_backward_update(const ComputeContext &context) const {
  dispatch_update_phase(context, m_backward_update_phase);
}

} // namespace MR::GPU
