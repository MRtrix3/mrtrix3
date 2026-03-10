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

#include "algo/threaded_loop.h"
#include "datatype.h"
#include "exception.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/registration/imageoperations.h"
#include "gpu/registration/nonlinearlncc.h"
#include "gpu/registration/voxelscannermatrices.h"
#include "header.h"
#include "interp/linear.h"
#include "transform.h"

#include <Eigen/Core>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
using namespace MR::GPU;

constexpr WorkgroupSize workgroup_size{.x = 8U, .y = 4U, .z = 4U};
// Scaling-and-squaring threshold in target voxel units.
// We choose the number of squaring steps so that max(||v / 2^n||) <= this value before composition.
constexpr float exponentiation_max_velocity_norm_voxels = 0.5F;
constexpr float update_alpha = 1.0F;
constexpr float update_epsilon = 1.0e-5F;
constexpr float update_max_magnitude = 0.5F; // in scanner space units (mm)
// Larger values make updates more conservative; smaller values allow more aggressive updates.
constexpr float lncc_sigma_ratio = 0.05F;
constexpr float lncc_moment_epsilon = 1.0e-6F;
constexpr float lncc_rho_epsilon = 1.0e-6F;
constexpr float lncc_denominator_epsilon = 1.0e-6F;
// Gaussian kernel truncation factor used to derive the effective radius of the local update kernels from their sigma
// values.
constexpr float gaussian_blur_k = 2.0F;
constexpr float fluid_blur_sigma = 1.5F; // in voxel units
const uint32_t fluid_blur_radius = static_cast<uint32_t>(std::ceil(gaussian_blur_k * fluid_blur_sigma));
constexpr float diffusion_blur_sigma = 1.5F; // in voxel units
const uint32_t diffusion_blur_radius = static_cast<uint32_t>(std::ceil(gaussian_blur_k * diffusion_blur_sigma));
constexpr float velocity_step_size = 1.0F;
constexpr float velocity_clamp_max = 0.5F; // in scanner space units (mm)
constexpr float velocity_clamp_epsilon = 1.0e-6F;
constexpr uint32_t num_levels = 3U;
constexpr uint32_t convergence_window = 5U;
constexpr float convergence_min_relative_improvement = 1.0e-5F;
constexpr float convergence_cost_floor = 1.0e-6F;
constexpr float inertia_weight = 0.5F;

struct DispatchGridUniforms {
  alignas(16) DispatchGrid dispatch_grid{};
};
static_assert(sizeof(DispatchGridUniforms) % 16 == 0, "DispatchGridUniforms must be 16-byte aligned.");

struct alignas(16) ExponentiateInitUniforms {
  uint32_t num_steps = 0U;
};
static_assert(sizeof(ExponentiateInitUniforms) % 16 == 0, "ExponentiateInitUniforms must be 16-byte aligned.");

class BackwardWarpThreadKernel {
public:
  BackwardWarpThreadKernel(MR::Image<float> &backward_displacement,
                           const Eigen::Matrix4f &moving_voxel_to_scanner_matrix)
      : backward_displacement(backward_displacement, 0.0F), moving_voxel_to_scanner(moving_voxel_to_scanner_matrix) {}

  void operator()(MR::Image<float> &warp) {
    const Eigen::Vector4f moving_voxel(
        static_cast<float>(warp.index(0)), static_cast<float>(warp.index(1)), static_cast<float>(warp.index(2)), 1.0F);
    const Eigen::Vector4f moving_scanner = moving_voxel_to_scanner * moving_voxel;
    Eigen::Vector3f displacement_scanner = Eigen::Vector3f::Zero();
    if (backward_displacement.scanner(moving_scanner.head<3>().cast<double>())) {
      displacement_scanner = backward_displacement.row(3).cast<float>();
    }
    const Eigen::Vector3f fixed_scanner = moving_scanner.head<3>() + displacement_scanner;
    warp.row(3) = fixed_scanner;
  }

private:
  MR::Interp::Linear<MR::Image<float>> backward_displacement;
  const Eigen::Matrix4f moving_voxel_to_scanner;
};

uint32_t compute_exponentiation_steps(float max_velocity_norm_voxels) {
  if (!(max_velocity_norm_voxels > exponentiation_max_velocity_norm_voxels)) {
    return 0U;
  }

  const float scale_ratio = max_velocity_norm_voxels / exponentiation_max_velocity_norm_voxels;
  const float required_steps = std::ceil(std::log2(scale_ratio));
  const uint32_t rounded_steps = required_steps > 0.0F ? static_cast<uint32_t>(required_steps) : 0U;
  // Force an even number of squaring steps so displacement ping-pong always ends in displacement1.
  return (rounded_steps % 2U) == 0U ? rounded_steps : (rounded_steps + 1U);
}

// Performs the scaling and squaring steps to exponentiate the velocity field, writing the output to the displacement
// texture. We use a ping-pong approach with two velocity and two displacement textures to avoid read-write conflicts.
// This is required because GPU kernels generally can’t safely read and write the same texture in one dispatch.
void exponentiate_velocity(const Kernel &init_kernel,
                           uint32_t num_steps,
                           const Kernel &square_12,
                           const Kernel &square_21,
                           const DispatchGrid &dispatch_grid,
                           const ComputeContext &context) {
  context.dispatch_kernel(init_kernel, dispatch_grid);

  bool displacement_is_1 = true;
  for (uint32_t step = 0U; step < num_steps; ++step) {
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

namespace MR::GPU {

NonLinearRegistrationResult run_nonlinear_registration(const NonLinearRegistrationConfig &config,
                                                       const GPU::ComputeContext &context) {
  // Symmetric log-domain non-linear registration pipeline with inertial local updates:
  // 1. Validate inputs and allocate GPU textures for velocity, displacement, and local updates.
  // 2. Iterate:
  //    - exponentiate v to get forward displacement exp(v) via scaling-and-squaring
  //    - compute forward metric cost and forward local demons update
  //    - negate velocity and exponentiate -v to get backward displacement exp(-v)
  //    - compute backward metric cost and backward local demons update
  //    - combine updates as u_raw = 0.5 * (u_fwd - u_bwd)
  //    - apply inertia as u = u_raw + alpha * u_prev
  //    - regularise u via Gaussian smoothing (fluid-like regularisation)
  //    - apply u to velocity
  //    - regularise velocity via Gaussian smoothing (diffusion-like regularisation)
  //    - evaluate convergence using symmetric metric: 0.5 * (cost_fwd + cost_bwd)
  // 3. Exponentiate the final velocity, download forward displacement, and write the output warp in scanner space.
  // See the following papers:
  // - Symmetric Log-Domain Diffeomorphic Registration: A Demons-based Approach by Vercauteren et al.
  // - Diffeomorphic demons: Efficient non-parametric image registration by Vercauteren et al.
  // - An ITK Implementation of the Symmetric Log-Domain Diffeomorphic Demons Algorithm by Dru & Vercauteren.

  if (config.channels.empty()) {
    throw Exception("At least one channel must be provided for registration");
  }

  const ChannelConfig &channel = config.channels.front();
  if (channel.image1Mask || channel.image2Mask) {
    throw Exception("Non-linear registration draft does not yet support masks.");
  }

  const NCCMetric *const ncc_metric = std::get_if<NCCMetric>(&config.metric);
  const bool use_lncc_metric = ncc_metric != nullptr;
  const uint32_t lncc_window_radius = use_lncc_metric ? ncc_metric->window_radius : 0U;
  if (use_lncc_metric && lncc_window_radius == 0U) {
    throw Exception("Non-linear registration: NCC metric requires ncc_radius >= 1.");
  }
  if (use_lncc_metric) {
    if(lncc_window_radius <= 0U) {
      throw Exception("Non-linear registration: invalid LNCC window radius.");
    }
    INFO("Non-linear registration: using LNCC with box radius " + std::to_string(lncc_window_radius));
  }

  const Texture moving_texture = context.new_texture_from_host_image(channel.image1);
  const Texture fixed_texture = context.new_texture_from_host_image(channel.image2);
  const std::vector<Texture> moving_pyramid =
      createDownsampledPyramid(moving_texture, static_cast<int32_t>(num_levels), context);
  const std::vector<Texture> fixed_pyramid =
      createDownsampledPyramid(fixed_texture, static_cast<int32_t>(num_levels), context);
  const Sampler linear_sampler = context.new_linear_sampler();
  std::optional<Texture> previous_level_velocity;
  std::optional<Texture> previous_level_fixed;
  std::optional<Texture> final_forward_displacement;
  std::optional<Texture> final_backward_displacement;

  for (uint32_t level = 0U; level < num_levels; ++level) {
    INFO("Non-linear registration: processing level " + std::to_string(level + 1U) + "/" + std::to_string(num_levels));
    const Texture &moving_level = moving_pyramid[level];
    const Texture &fixed_level = fixed_pyramid[level];
    const float level_downscale = std::exp2f(static_cast<float>(num_levels - 1U - level));

    const TextureSpec vector_texture_spec{
        .width = fixed_level.spec.width,
        .height = fixed_level.spec.height,
        .depth = fixed_level.spec.depth,
        .format = TextureFormat::RGBA32Float,
        .usage = {.storage_binding = true, .render_target = false},
    };
    const TextureSpec moving_vector_texture_spec{
        .width = moving_level.spec.width,
        .height = moving_level.spec.height,
        .depth = moving_level.spec.depth,
        .format = TextureFormat::RGBA32Float,
        .usage = {.storage_binding = true, .render_target = false},
    };
    const VoxelScannerMatrices voxel_scanner_matrices =
        VoxelScannerMatrices::from_image_pair(channel.image1, channel.image2, level_downscale);
    const Buffer<std::byte> voxel_scanner_buffer =
        context.new_buffer_from_host_object(voxel_scanner_matrices, BufferType::UniformBuffer);
    const Texture velocity1 = context.new_empty_texture(vector_texture_spec);
    const Texture velocity2 = context.new_empty_texture(vector_texture_spec);
    const Texture negated_velocity = context.new_empty_texture(vector_texture_spec);
    const Texture displacement1 = context.new_empty_texture(vector_texture_spec);
    const Texture displacement2 = context.new_empty_texture(vector_texture_spec);
    const Texture forward_displacement_cache = context.new_empty_texture(vector_texture_spec);
    const Texture forward_update_field = context.new_empty_texture(vector_texture_spec);
    const Texture backward_update_field_moving = context.new_empty_texture(moving_vector_texture_spec);
    const Texture backward_update_field = context.new_empty_texture(vector_texture_spec);
    const Texture inertial_update_field = context.new_empty_texture(vector_texture_spec);
    const Texture smoothed_update_field = context.new_empty_texture(vector_texture_spec);

    const DispatchGrid dispatch_grid = DispatchGrid::element_wise_texture(fixed_level, workgroup_size);
    const DispatchGrid moving_dispatch_grid = DispatchGrid::element_wise_texture(moving_level, workgroup_size);
    const GaussianSmoothingParams fluid_smoothing_params{
        .radius = fluid_blur_radius,
        .sigma = fluid_blur_sigma,
    };
    const GaussianSmoothingParams diffusion_smoothing_params{
        .radius = diffusion_blur_radius,
        .sigma = diffusion_blur_sigma,
    };
    const bool use_fluid_smoothing = fluid_blur_sigma != 0.0F;
    const bool use_diffusion_smoothing = diffusion_blur_sigma != 0.0F;
    std::optional<SeparableGaussianBlurPipeline> fluid_update_blur;
    std::optional<SeparableGaussianBlurPipeline> diffusion_velocity_blur_12;
    std::optional<SeparableGaussianBlurPipeline> diffusion_velocity_blur_21;
    if (use_fluid_smoothing) {
      fluid_update_blur.emplace(context, inertial_update_field, smoothed_update_field, fluid_smoothing_params);
    }
    if (use_diffusion_smoothing) {
      diffusion_velocity_blur_12.emplace(context, velocity1, velocity2, diffusion_smoothing_params);
      diffusion_velocity_blur_21.emplace(context, velocity2, velocity1, diffusion_smoothing_params);
    }

    const ShaderEntry exp_init_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/exponentiate.slang"},
        .entryPoint = "initialise",
        .workgroup_size = workgroup_size,
    };
    const Buffer<std::byte> exp_init_uniforms_buffer =
        context.new_buffer_from_host_object(ExponentiateInitUniforms{}, BufferType::UniformBuffer);
    // We always use an even number of squaring steps, so initialising into displacement1 is sufficient.
    const Kernel exp_init_from_v1 = context.new_kernel({.compute_shader = exp_init_shader,
                                                        .bindings_map = {{"uniforms", exp_init_uniforms_buffer},
                                                                         {"velocityTexture", velocity1},
                                                                         {"output", displacement1}}});
    const Kernel exp_init_from_v2 = context.new_kernel({.compute_shader = exp_init_shader,
                                                        .bindings_map = {{"uniforms", exp_init_uniforms_buffer},
                                                                         {"velocityTexture", velocity2},
                                                                         {"output", displacement1}}});
    const Kernel exp_init_from_negated = context.new_kernel({.compute_shader = exp_init_shader,
                                                             .bindings_map = {{"uniforms", exp_init_uniforms_buffer},
                                                                              {"velocityTexture", negated_velocity},
                                                                              {"output", displacement1}}});

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
                                                                      {"voxelScannerMatrices", voxel_scanner_buffer},
                                                                      {"linearSampler", linear_sampler}}});
    const Kernel exp_square_21 = context.new_kernel({.compute_shader = exp_square_shader,
                                                     .bindings_map = {{"inputDisplacement", displacement2},
                                                                      {"outputDisplacement", displacement1},
                                                                      {"voxelScannerMatrices", voxel_scanner_buffer},
                                                                      {"linearSampler", linear_sampler}}});

    const ShaderEntry ssd_forward_update_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/local_ssd_update.slang"},
        .entryPoint = "ssd_update_forward",
        .workgroup_size = workgroup_size,
        .constants = {{"kAlpha", update_alpha},
                      {"kEpsilon", update_epsilon},
                      {"kMaxUpdateMagnitude", update_max_magnitude}},
    };
    const Kernel ssd_forward_update_kernel =
        context.new_kernel({.compute_shader = ssd_forward_update_shader,
                            .bindings_map = {{"fixedImage", fixed_level},
                                             {"movingImage", moving_level},
                                             {"displacement", displacement1},
                                             {"voxelScannerMatrices", voxel_scanner_buffer},
                                             {"outputUpdate", forward_update_field},
                                             {"linearSampler", linear_sampler}}});
    const ShaderEntry ssd_backward_update_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/local_ssd_update.slang"},
        .entryPoint = "ssd_update_backward",
        .workgroup_size = workgroup_size,
        .constants = {{"kAlpha", update_alpha},
                      {"kEpsilon", update_epsilon},
                      {"kMaxUpdateMagnitude", update_max_magnitude}},
    };
    const Kernel ssd_backward_update_kernel =
        context.new_kernel({.compute_shader = ssd_backward_update_shader,
                            .bindings_map = {{"fixedImage", fixed_level},
                                             {"movingImage", moving_level},
                                             {"displacementBackward", displacement1},
                                             {"voxelScannerMatrices", voxel_scanner_buffer},
                                             {"outputUpdate", backward_update_field_moving},
                                             {"linearSampler", linear_sampler}}});
    const ShaderEntry resample_backward_update_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/resample_update_moving_to_fixed.slang"},
        .entryPoint = "main",
        .workgroup_size = workgroup_size,
    };
    const Kernel resample_backward_update_kernel =
        context.new_kernel({.compute_shader = resample_backward_update_shader,
                            .bindings_map = {{"movingUpdate", backward_update_field_moving},
                                             {"forwardDisplacement", forward_displacement_cache},
                                             {"voxelScannerMatrices", voxel_scanner_buffer},
                                             {"outputUpdate", backward_update_field},
                                             {"linearSampler", linear_sampler}}});

    const ShaderEntry apply_update_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/apply_velocity_update.slang"},
        .entryPoint = "main",
        .workgroup_size = workgroup_size,
        .constants =
            {
                {"kStepSize", velocity_step_size},
                {"kMaxUpdateMagnitude", velocity_clamp_max},
                {"kClampEpsilon", velocity_clamp_epsilon},
            },
    };
    const Texture &velocity_update_field = use_fluid_smoothing ? smoothed_update_field : inertial_update_field;
    const Kernel apply_update_12 = context.new_kernel({
        .compute_shader = apply_update_shader,
        .bindings_map = {{"velocity", velocity1}, {"update", velocity_update_field}, {"outputVelocity", velocity2}},
    });
    const Kernel apply_update_21 = context.new_kernel({
        .compute_shader = apply_update_shader,
        .bindings_map = {{"velocity", velocity2}, {"update", velocity_update_field}, {"outputVelocity", velocity1}},
    });

    const ShaderEntry ssd_forward_cost_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/ssd_cost.slang"},
        .entryPoint = "ssd_cost_forward",
        .workgroup_size = workgroup_size,
        .constants = {{"kIncludeOutOfBounds", uint32_t{1}}},
    };
    const ShaderEntry ssd_backward_cost_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/ssd_cost.slang"},
        .entryPoint = "ssd_cost_backward",
        .workgroup_size = workgroup_size,
        .constants = {{"kIncludeOutOfBounds", uint32_t{1}}},
    };
    const DispatchGrid backward_cost_dispatch_grid = moving_dispatch_grid;
    const DispatchGridUniforms forward_dispatch_uniforms{
        .dispatch_grid = dispatch_grid,
    };
    const DispatchGridUniforms backward_dispatch_uniforms{
        .dispatch_grid = backward_cost_dispatch_grid,
    };
    const Buffer<std::byte> forward_dispatch_uniforms_buffer =
        context.new_buffer_from_host_object(forward_dispatch_uniforms, BufferType::UniformBuffer);
    const Buffer<std::byte> backward_dispatch_uniforms_buffer =
        context.new_buffer_from_host_object(backward_dispatch_uniforms, BufferType::UniformBuffer);
    const Buffer<float> ssd_forward_cost_partials_buffer =
        context.new_empty_buffer<float>(dispatch_grid.workgroup_count());
    const Buffer<float> ssd_forward_cost_counts_buffer =
        context.new_empty_buffer<float>(dispatch_grid.workgroup_count());
    const Buffer<float> ssd_backward_cost_partials_buffer =
        context.new_empty_buffer<float>(backward_cost_dispatch_grid.workgroup_count());
    const Buffer<float> ssd_backward_cost_counts_buffer =
        context.new_empty_buffer<float>(backward_cost_dispatch_grid.workgroup_count());
    const Kernel ssd_forward_cost_kernel = context.new_kernel({.compute_shader = ssd_forward_cost_shader,
                                                               .bindings_map = {
                                                                   {"uniforms", forward_dispatch_uniforms_buffer},
                                                                   {"fixedImage", fixed_level},
                                                                   {"movingImage", moving_level},
                                                                   {"displacement", displacement1},
                                                                   {"voxelScannerMatrices", voxel_scanner_buffer},
                                                                   {"linearSampler", linear_sampler},
                                                                   {"ssdPartials", ssd_forward_cost_partials_buffer},
                                                                   {"ssdCounts", ssd_forward_cost_counts_buffer},
                                                               }});
    const Kernel ssd_backward_cost_kernel = context.new_kernel({.compute_shader = ssd_backward_cost_shader,
                                                                .bindings_map = {
                                                                    {"uniforms", backward_dispatch_uniforms_buffer},
                                                                    {"fixedImage", fixed_level},
                                                                    {"movingImage", moving_level},
                                                                    {"displacementBackward", displacement1},
                                                                    {"voxelScannerMatrices", voxel_scanner_buffer},
                                                                    {"linearSampler", linear_sampler},
                                                                    {"ssdPartials", ssd_backward_cost_partials_buffer},
                                                                    {"ssdCounts", ssd_backward_cost_counts_buffer},
                                                                }});
    std::optional<NonlinearLnccPipeline> lncc_pipeline;
    if (use_lncc_metric) {
      const NonlinearLnccPipelineConfig lncc_pipeline_config{
          .window_radius = lncc_window_radius,
          .sigma_ratio = lncc_sigma_ratio,
          .moment_epsilon = lncc_moment_epsilon,
          .rho_epsilon = lncc_rho_epsilon,
          .denominator_epsilon = lncc_denominator_epsilon,
          .max_update_magnitude = update_max_magnitude,
          .fixed_moments_texture_spec = vector_texture_spec,
          .moving_moments_texture_spec = moving_vector_texture_spec,
          .fixed_image = fixed_level,
          .moving_image = moving_level,
          .displacement = displacement1,
          .forward_dispatch_uniforms_buffer = forward_dispatch_uniforms_buffer,
          .backward_dispatch_uniforms_buffer = backward_dispatch_uniforms_buffer,
          .voxel_scanner_buffer = voxel_scanner_buffer,
          .linear_sampler = linear_sampler,
          .forward_dispatch_grid = dispatch_grid,
          .backward_dispatch_grid = backward_cost_dispatch_grid,
          .forward_update_output = forward_update_field,
          .backward_update_output = backward_update_field_moving,
      };
      lncc_pipeline.emplace(context, lncc_pipeline_config);
    }
    const ShaderEntry negate_velocity_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/negate_velocity.slang"},
        .entryPoint = "main",
        .workgroup_size = workgroup_size,
    };
    const Kernel negate_velocity_v1 =
        context.new_kernel({.compute_shader = negate_velocity_shader,
                            .bindings_map = {{"velocity", velocity1}, {"outputVelocity", negated_velocity}}});
    const Kernel negate_velocity_v2 =
        context.new_kernel({.compute_shader = negate_velocity_shader,
                            .bindings_map = {{"velocity", velocity2}, {"outputVelocity", negated_velocity}}});
    const ShaderEntry symmetric_combine_updates_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/symmetric_combine_updates.slang"},
        .entryPoint = "main",
        .workgroup_size = workgroup_size,
        .constants = {{"kInertiaWeight", inertia_weight}},
    };
    const Kernel symmetric_combine_updates_kernel = context.new_kernel({
        .compute_shader = symmetric_combine_updates_shader,
        .bindings_map = {{"forwardUpdate", forward_update_field},
                         {"backwardUpdate", backward_update_field},
                         {"update", inertial_update_field}},
    });
    const ShaderEntry velocity_max_norm_shader{
        .shader_source = ShaderFile{"shaders/registration/nonlinear/velocity_max_norm.slang"},
        .entryPoint = "main",
        .workgroup_size = workgroup_size,
    };
    const Buffer<float> velocity_max_norm_sq_partials_buffer =
        context.new_empty_buffer<float>(dispatch_grid.workgroup_count());
    const Kernel velocity_max_norm_v1_kernel =
        context.new_kernel({.compute_shader = velocity_max_norm_shader,
                            .bindings_map = {{"uniforms", forward_dispatch_uniforms_buffer},
                                             {"velocityTexture", velocity1},
                                             {"voxelScannerMatrices", voxel_scanner_buffer},
                                             {"maxNormSqPartials", velocity_max_norm_sq_partials_buffer}}});
    const Kernel velocity_max_norm_v2_kernel = context.new_kernel({
        .compute_shader = velocity_max_norm_shader,
        .bindings_map = {{"uniforms", forward_dispatch_uniforms_buffer},
                         {"velocityTexture", velocity2},
                         {"voxelScannerMatrices", voxel_scanner_buffer},
                         {"maxNormSqPartials", velocity_max_norm_sq_partials_buffer}},
    });
    const auto evaluate_symmetric_cost = [&](const Kernel &cost_kernel,
                                             const DispatchGrid &cost_dispatch_grid,
                                             const Buffer<float> &cost_partials_buffer,
                                             const Buffer<float> &cost_counts_buffer) {
      context.dispatch_kernel(cost_kernel, cost_dispatch_grid);
      return evaluate_cost_from_partials(context, cost_partials_buffer, cost_counts_buffer);
    };
    const auto evaluate_exponentiation_steps = [&](const Kernel &velocity_max_norm_kernel) {
      context.dispatch_kernel(velocity_max_norm_kernel, dispatch_grid);
      const std::vector<float> partial_max_norm_sq =
          context.download_buffer_as_vector(velocity_max_norm_sq_partials_buffer);
      const auto max_norm_sq_it = std::max_element(partial_max_norm_sq.begin(), partial_max_norm_sq.end());
      const float max_norm_sq = max_norm_sq_it == partial_max_norm_sq.end() ? 0.0F : *max_norm_sq_it;
      const float max_norm = std::sqrt(std::max(0.0F, max_norm_sq));
      return compute_exponentiation_steps(max_norm);
    };
    const auto exponentiate_from_kernel = [&](const Kernel &exp_init_kernel, uint32_t num_steps) {
      const ExponentiateInitUniforms exp_init_uniforms{
          .num_steps = num_steps,
      };
      context.write_object_to_buffer(exp_init_uniforms_buffer, exp_init_uniforms);
      exponentiate_velocity(exp_init_kernel, num_steps, exp_square_12, exp_square_21, dispatch_grid, context);
    };

    bool velocity_is_1 = true;
    if (previous_level_velocity && previous_level_fixed) {
      const float scale_x =
          static_cast<float>(fixed_level.spec.width) / static_cast<float>(previous_level_fixed->spec.width);
      const float scale_y =
          static_cast<float>(fixed_level.spec.height) / static_cast<float>(previous_level_fixed->spec.height);
      const float scale_z =
          static_cast<float>(fixed_level.spec.depth) / static_cast<float>(previous_level_fixed->spec.depth);
      const ShaderEntry upsample_shader{
          .shader_source = ShaderFile{"shaders/registration/nonlinear/upsample_velocity.slang"},
          .entryPoint = "main",
          .workgroup_size = workgroup_size,
          .constants = {{"kScaleX", scale_x}, {"kScaleY", scale_y}, {"kScaleZ", scale_z}},
      };
      const Kernel upsample_kernel = context.new_kernel({.compute_shader = upsample_shader,
                                                         .bindings_map = {{"coarseVelocity", *previous_level_velocity},
                                                                          {"outputVelocity", velocity1},
                                                                          {"linearSampler", linear_sampler}}});
      context.dispatch_kernel(upsample_kernel, dispatch_grid);
    }

    bool converged_this_level = false;
    std::vector<float> recent_costs;
    recent_costs.reserve(convergence_window + 1U);
    for (uint32_t iter = 0U; iter < config.max_iterations; ++iter) {
      const Kernel &velocity_max_norm_kernel =
          velocity_is_1 ? velocity_max_norm_v1_kernel : velocity_max_norm_v2_kernel;
      const Kernel &exp_init_kernel = velocity_is_1 ? exp_init_from_v1 : exp_init_from_v2;
      const uint32_t exponentiation_steps = evaluate_exponentiation_steps(velocity_max_norm_kernel);
      exponentiate_from_kernel(exp_init_kernel, exponentiation_steps);
      const float forward_cost = use_lncc_metric ? lncc_pipeline->evaluate_forward_cost(context)
                                                 : evaluate_symmetric_cost(ssd_forward_cost_kernel,
                                                                           dispatch_grid,
                                                                           ssd_forward_cost_partials_buffer,
                                                                           ssd_forward_cost_counts_buffer);
      if (use_lncc_metric) {
        lncc_pipeline->compute_forward_update(context);
      } else {
        context.dispatch_kernel(ssd_forward_update_kernel, dispatch_grid);
      }
      context.copy_texture_to_texture(displacement1, forward_displacement_cache, {});

      const Kernel &negate_velocity_kernel = velocity_is_1 ? negate_velocity_v1 : negate_velocity_v2;
      context.dispatch_kernel(negate_velocity_kernel, dispatch_grid);

      exponentiate_from_kernel(exp_init_from_negated, exponentiation_steps);
      const float backward_cost = use_lncc_metric ? lncc_pipeline->evaluate_backward_cost(context)
                                                  : evaluate_symmetric_cost(ssd_backward_cost_kernel,
                                                                            backward_cost_dispatch_grid,
                                                                            ssd_backward_cost_partials_buffer,
                                                                            ssd_backward_cost_counts_buffer);
      if (use_lncc_metric) {
        lncc_pipeline->compute_backward_update(context);
      } else {
        context.dispatch_kernel(ssd_backward_update_kernel, moving_dispatch_grid);
      }
      context.dispatch_kernel(resample_backward_update_kernel, dispatch_grid);

      const float cost = 0.5F * (forward_cost + backward_cost);
      recent_costs.push_back(cost);
      if (recent_costs.size() > (convergence_window + 1U)) {
        recent_costs.erase(recent_costs.begin());
      }
      INFO("Non-linear registration: level " + std::to_string(level + 1U) + "/" + std::to_string(num_levels) +
           " iteration " + std::to_string(iter + 1U) + "/" + std::to_string(config.max_iterations) +
           " cost_fwd=" + std::to_string(forward_cost) + " cost_bwd=" + std::to_string(backward_cost) +
           " cost_sym=" + std::to_string(cost));

      // Compute the relative improvement over the convergence window
      if (recent_costs.size() == (convergence_window + 1U)) {
        const float window_start_cost = recent_costs.front();
        const float window_end_cost = recent_costs.back();
        const float absolute_start_cost = std::fabs(window_start_cost);
        const float denominator =
            absolute_start_cost > convergence_cost_floor ? absolute_start_cost : convergence_cost_floor;
        const float relative_improvement = (window_start_cost - window_end_cost) / denominator;
        if (relative_improvement < convergence_min_relative_improvement) {
          converged_this_level = true;
          CONSOLE("Non-linear registration: convergence reached at level " + std::to_string(level + 1U) + "/" +
                  std::to_string(num_levels) + " after " + std::to_string(iter + 1U) +
                  " iterations (relative cost improvement over last " + std::to_string(convergence_window) +
                  " iterations: " + std::to_string(relative_improvement) + ").");
          break;
        }
      }

      context.dispatch_kernel(symmetric_combine_updates_kernel, dispatch_grid);
      if (fluid_update_blur) {
        fluid_update_blur->run(context);
      }
      const Kernel &apply_update_kernel = velocity_is_1 ? apply_update_12 : apply_update_21;
      context.dispatch_kernel(apply_update_kernel, dispatch_grid);

      // After the local update is applied, smooth the velocity field itself (diffusion regularisation).
      velocity_is_1 = !velocity_is_1;
      if (use_diffusion_smoothing) {
        const auto &diffusion_blur = velocity_is_1 ? *diffusion_velocity_blur_12 : *diffusion_velocity_blur_21;
        diffusion_blur.run(context);
        velocity_is_1 = !velocity_is_1;
      }
    }
    if (!converged_this_level) {
      CONSOLE("Non-linear registration: max iterations reached without convergence at level " +
              std::to_string(level + 1U) + "/" + std::to_string(num_levels) + " (" +
              std::to_string(config.max_iterations) + " iterations).");
    }

    if (level == (num_levels - 1U)) {
      const Kernel &velocity_max_norm_kernel =
          velocity_is_1 ? velocity_max_norm_v1_kernel : velocity_max_norm_v2_kernel;
      const Kernel &exp_init_kernel = velocity_is_1 ? exp_init_from_v1 : exp_init_from_v2;
      const uint32_t exponentiation_steps = evaluate_exponentiation_steps(velocity_max_norm_kernel);
      exponentiate_from_kernel(exp_init_kernel, exponentiation_steps);
      context.copy_texture_to_texture(displacement1, forward_displacement_cache, {});

      const Kernel &negate_velocity_kernel = velocity_is_1 ? negate_velocity_v1 : negate_velocity_v2;
      context.dispatch_kernel(negate_velocity_kernel, dispatch_grid);
      exponentiate_from_kernel(exp_init_from_negated, exponentiation_steps);

      final_forward_displacement = forward_displacement_cache;
      final_backward_displacement = displacement1;
    } else {
      previous_level_velocity = velocity_is_1 ? velocity1 : velocity2;
      previous_level_fixed = fixed_level;
    }
  }

  if (!final_forward_displacement || !final_backward_displacement) {
    throw Exception("Non-linear registration: final forward/backward displacements were not computed.");
  }

  Header forward_displacement_header(channel.image2);
  forward_displacement_header.ndim() = 4;
  forward_displacement_header.size(3) = 3;
  forward_displacement_header.datatype() = DataType::from<float>();
  Image<float> forward_displacement_image =
      context.download_texture_as_image(*final_forward_displacement,
                                        forward_displacement_header,
                                        "nonlinear forward displacement",
                                        ComputeContext::DownloadTextureAlphaMode::IgnoreAlpha);

  Header backward_displacement_header(channel.image2);
  backward_displacement_header.ndim() = 4;
  backward_displacement_header.size(3) = 3;
  backward_displacement_header.datatype() = DataType::from<float>();
  Image<float> backward_displacement_image =
      context.download_texture_as_image(*final_backward_displacement,
                                        backward_displacement_header,
                                        "nonlinear backward displacement",
                                        ComputeContext::DownloadTextureAlphaMode::IgnoreAlpha);

  Header warp1_header(channel.image2);
  warp1_header.ndim() = 4;
  warp1_header.size(3) = 3;
  warp1_header.datatype() = DataType::from<float>();

  Image<float> warp1_image = Image<float>::scratch(warp1_header, "nonlinear warp1").with_direct_io();
  const Transform fixed_transform(channel.image2);
  const Eigen::Matrix4f fixed_voxel_to_scanner = EigenHelpers::to_homogeneous_mat4f(fixed_transform.voxel2scanner);

  auto write_warp = [&](Image<float> &warp, Image<float> &displacement) {
    const Eigen::Vector3f displacement_scanner(displacement.row(3));
    const Eigen::Vector4f fixed_voxel(
        static_cast<float>(warp.index(0)), static_cast<float>(warp.index(1)), static_cast<float>(warp.index(2)), 1.0F);
    const Eigen::Vector4f fixed_scanner = fixed_voxel_to_scanner * fixed_voxel;
    const Eigen::Vector3f moving_scanner = fixed_scanner.head<3>() + displacement_scanner;
    warp.row(3) = moving_scanner;
  };
  ThreadedLoop("writing nonlinear warp1", warp1_image, 0, 3).run(write_warp, warp1_image, forward_displacement_image);

  Header warp2_header(channel.image1);
  warp2_header.ndim() = 4;
  warp2_header.size(3) = 3;
  warp2_header.datatype() = DataType::from<float>();
  Image<float> warp2_image = Image<float>::scratch(warp2_header, "nonlinear warp2").with_direct_io();
  const Transform moving_transform(channel.image1);
  const Eigen::Matrix4f moving_voxel_to_scanner = EigenHelpers::to_homogeneous_mat4f(moving_transform.voxel2scanner);
  // warp2 is defined as a deformation field on image1 voxel grid, but the GPU backward displacement we have at the end
  // is stored on the image2 (fixed) grid.
  ThreadedLoop("writing nonlinear warp2", warp2_image, 0, 3)
      .run(BackwardWarpThreadKernel(backward_displacement_image, moving_voxel_to_scanner), warp2_image);

  return NonLinearRegistrationResult{
      .warp1 = std::move(warp1_image),
      .warp2 = std::move(warp2_image),
  };
}

} // namespace MR::GPU
