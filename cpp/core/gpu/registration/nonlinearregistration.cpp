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

#include "gpu/registration/nonlinearregistration.h"

#include "algo/loop.h"
#include "datatype.h"
#include "exception.h"
#include "gpu/registration/eigenhelpers.h"
#include "transform.h"

#include <tcb/span.hpp>

#include <Eigen/Core>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace {
using namespace MR::GPU;

constexpr WorkgroupSize workgroup_size{.x = 8U, .y = 4U, .z = 4U};
constexpr uint32_t exponentiate_steps = 6U;
constexpr float update_alpha = 1.0F;
constexpr float update_epsilon = 1.0e-5F;
constexpr float update_max_magnitude = 0.5F;
constexpr float velocity_step_size = 0.25F;
constexpr float velocity_clamp_max = 0.5F;
constexpr float velocity_clamp_epsilon = 1.0e-6F;

// Performs the scaling and squaring steps to exponentiate the velocity field, writing the output to the displacement texture.
// We use a ping-pong approach with two velocity and two displacement textures to avoid read-write conflicts.
void exponentiate_velocity(bool velocity_is_1,
                           const Kernel &init_from_v1,
                           const Kernel &init_from_v2,
                           const Kernel &square_12,
                           const Kernel &square_21,
                           const DispatchGrid &dispatch_grid,
                           const ComputeContext &context) {
  if (velocity_is_1) {
    context.dispatch_kernel(init_from_v1, dispatch_grid);
  } else {
    context.dispatch_kernel(init_from_v2, dispatch_grid);
  }

  bool displacement_is_1 = true;
  for (uint32_t step = 0U; step < exponentiate_steps; ++step) {
    if (displacement_is_1) {
      context.dispatch_kernel(square_12, dispatch_grid);
    } else {
      context.dispatch_kernel(square_21, dispatch_grid);
    }
    displacement_is_1 = !displacement_is_1;
  }

  if (!displacement_is_1) {
    throw MR::Exception("Non-linear registration: displacement ping-pong mismatch.");
  }
}

} // namespace

namespace MR::GPU {

NonLinearRegistrationResult run_nonlinear_registration(const NonLinearRegistrationConfig &config,
                                                       const GPU::ComputeContext &context) {
  // Draft non-linear registration pipeline:
  // 1. Validate inputs and allocate GPU textures for velocity, displacement, and local updates.
  // 2. Iterate: exponentiate the current velocity (scaling and squaring), compute an SSD-based update,
  //    then apply that update back to the velocity field.
  // 3. Exponentiate the final velocity, download displacement, and write the output warp in scanner space.
  if (config.channels.empty()) {
    throw Exception("At least one channel must be provided for registration");
  }

  const ChannelConfig &channel = config.channels.front();
  if (channel.image1Mask || channel.image2Mask) {
    throw Exception("Non-linear registration draft does not yet support masks.");
  }

  if (!std::holds_alternative<SSDMetric>(config.metric)) {
    throw Exception("Non-linear registration draft currently supports SSD metric only.");
  }

  const Texture moving_texture = context.new_texture_from_host_image(channel.image1);
  const Texture fixed_texture = context.new_texture_from_host_image(channel.image2);

  const TextureSpec vector_texture_spec{
      .width = fixed_texture.spec.width,
      .height = fixed_texture.spec.height,
      .depth = fixed_texture.spec.depth,
      .format = TextureFormat::RGBA32Float,
      .usage = {.storage_binding = true, .render_target = false},
  };
  const size_t voxel_count =
      static_cast<size_t>(fixed_texture.spec.width) * fixed_texture.spec.height * fixed_texture.spec.depth;
  const Transform moving_transform(channel.image1);
  const Texture velocity1 = context.new_empty_texture(vector_texture_spec);
  const Texture velocity2 = context.new_empty_texture(vector_texture_spec);
  const Texture displacement1 = context.new_empty_texture(vector_texture_spec);
  const Texture displacement2 = context.new_empty_texture(vector_texture_spec);
  const Texture update_field = context.new_empty_texture(vector_texture_spec);

  const Sampler linear_sampler = context.new_linear_sampler();
  const DispatchGrid dispatch_grid = DispatchGrid::element_wise_texture(fixed_texture, workgroup_size);

  const ShaderEntry exp_init_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/exponentiate.slang"},
      .entryPoint = "initialise",
      .workgroup_size = workgroup_size,
      .constants = {{"kNumSteps", exponentiate_steps}},
  };
  // Velocity ping-pongs between two textures, so we keep one init kernel per source texture.
  const Kernel exp_init_from_v1 = context.new_kernel(
      {.compute_shader = exp_init_shader, .bindings_map = {{"velocityTexture", velocity1}, {"output", displacement1}}});
  const Kernel exp_init_from_v2 = context.new_kernel(
      {.compute_shader = exp_init_shader, .bindings_map = {{"velocityTexture", velocity2}, {"output", displacement1}}});

  const ShaderEntry exp_square_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/exponentiate.slang"},
      .entryPoint = "square",
      .workgroup_size = workgroup_size,
  };
  // Squaring kernel cannot read and write the same displacement texture in one dispatch.
  // These two kernels implement ping-pong writes: 1->2 then 2->1.
  const Kernel exp_square_12 = context.new_kernel({.compute_shader = exp_square_shader,
                                                   .bindings_map = {{"inputDisplacement", displacement1},
                                                                    {"outputDisplacement", displacement2},
                                                                    {"linearSampler", linear_sampler}}});
  const Kernel exp_square_21 = context.new_kernel({.compute_shader = exp_square_shader,
                                                   .bindings_map = {{"inputDisplacement", displacement2},
                                                                    {"outputDisplacement", displacement1},
                                                                    {"linearSampler", linear_sampler}}});

  const ShaderEntry ssd_update_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/local_ssd_update.slang"},
      .entryPoint = "main",
      .workgroup_size = workgroup_size,
      .constants = {{"kAlpha", update_alpha},
                    {"kEpsilon", update_epsilon},
                    {"kMaxUpdateMagnitude", update_max_magnitude}},
  };
  const Kernel ssd_update_kernel = context.new_kernel({.compute_shader = ssd_update_shader,
                                                       .bindings_map = {{"fixedImage", fixed_texture},
                                                                        {"movingImage", moving_texture},
                                                                        {"displacement", displacement1},
                                                                        {"outputUpdate", update_field},
                                                                        {"linearSampler", linear_sampler}}});

  const ShaderEntry apply_update_shader{
      .shader_source = ShaderFile{"shaders/registration/nonlinear/apply_velocity_update.slang"},
      .entryPoint = "main",
      .workgroup_size = workgroup_size,
      .constants = {{"kStepSize", velocity_step_size},
                    {"kMaxUpdateMagnitude", velocity_clamp_max},
                    {"kClampEpsilon", velocity_clamp_epsilon}},
  };
  const Kernel apply_update_12 = context.new_kernel(
      {.compute_shader = apply_update_shader,
       .bindings_map = {{"velocity", velocity1}, {"update", update_field}, {"outputVelocity", velocity2}}});
  const Kernel apply_update_21 = context.new_kernel(
      {.compute_shader = apply_update_shader,
       .bindings_map = {{"velocity", velocity2}, {"update", update_field}, {"outputVelocity", velocity1}}});

  bool velocity_is_1 = true;
  for (uint32_t iter = 0U; iter < config.max_iterations; ++iter) {
    exponentiate_velocity(
        velocity_is_1, exp_init_from_v1, exp_init_from_v2, exp_square_12, exp_square_21, dispatch_grid, context);
    context.dispatch_kernel(ssd_update_kernel, dispatch_grid);
    if (velocity_is_1) {
      context.dispatch_kernel(apply_update_12, dispatch_grid);
    } else {
      context.dispatch_kernel(apply_update_21, dispatch_grid);
    }
    velocity_is_1 = !velocity_is_1;
  }

  exponentiate_velocity(
      velocity_is_1, exp_init_from_v1, exp_init_from_v2, exp_square_12, exp_square_21, dispatch_grid, context);

  std::vector<float> displacement_host(voxel_count * 4U, 0.0F);
  context.download_texture(displacement1, tcb::span<float>(displacement_host.data(), displacement_host.size()));

  Header warp_header(channel.image2);
  warp_header.ndim() = 4;
  warp_header.size(3) = 3;
  warp_header.datatype() = DataType::from<default_type>();
  Image<default_type> warp_image = Image<default_type>::scratch(warp_header, "nonlinear warp").with_direct_io();
  // TODO: write warp
  return NonLinearRegistrationResult{.warp = std::move(warp_image)};
}

} // namespace MR::GPU
