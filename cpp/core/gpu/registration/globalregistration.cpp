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

#include "gpu/registration/globalregistration.h"
#include "gpu/registration/adabelief.h"
#include "gpu/registration/calculatorinterface.h"
#include "gpu/registration/convergencechecker.h"
#include "gpu/registration/eigenhelpers.h"
#include "exception.h"
#include "gpu/gpu.h"
#include "header.h"
#include "image.h"
#include "gpu/registration/imageoperations.h"
#include "gpu/registration/initialisation.h"
#include "match_variant.h"
#include "gpu/registration/ncccalculator.h"
#include "gpu/registration/nmicalculator.h"
#include "gpu/registration/registrationtypes.h"
#include <tcb/span.hpp>
#include "gpu/registration/ssdcalculator.h"
#include "gpu/registration/voxelscannermatrices.h"

#include <Eigen/Core>
#include <unsupported/Eigen/MatrixFunctions>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace MR;
using namespace MR::GPU;

constexpr float base_learning_rate = 0.1F;
// Threshold for considering translation parameters to have changed significantly (in mm)
constexpr float translation_significant_threshold = 1e-2F;
// Patience limits for convergence checking
// We want a higher patience on the coarsest level to get a good initial alignment
constexpr uint32_t coarsest_level_patience = 10U;
constexpr uint32_t finer_levels_patience = 5U;

namespace {
AdaBelief create_optimiser(tcb::span<const float> initial_params, float translation_learning_rate) {
  std::vector<AdaBelief::Parameter> optimization_parameters(initial_params.size());
  for (size_t i = 0; i < optimization_parameters.size(); ++i) {
    auto &param = optimization_parameters[i];
    param.value = initial_params[i];
    // Non-translation parameters have smaller learning rate to account for their larger impact on the transformation
    param.learning_rate = i < 3 ? translation_learning_rate : translation_learning_rate * 1e-3F;
  }
  return AdaBelief(optimization_parameters);
}

std::vector<float> make_convergence_thresholds(size_t param_count) {
  std::vector<float> thresholds(param_count);
  for (size_t i = 0; i < param_count; ++i) {
    thresholds[i] = (i < 3) ? translation_significant_threshold : translation_significant_threshold * 1e-2F;
  }
  return thresholds;
}
} // namespace

namespace MR::GPU {

struct WeightedGradients {
  explicit WeightedGradients(size_t N) : m_gradients(N, 0.0F) {}
  void add(const std::vector<float> &gradients, float weight) {
    if (gradients.size() != m_gradients.size()) {
      throw std::logic_error("WeightedGradients::add: gradient size mismatch");
    }
    for (size_t i = 0; i < gradients.size(); ++i) {
      m_gradients[i] += weight * gradients[i];
    }
  }

  const std::vector<float> &get() const { return m_gradients; }

private:
  std::vector<float> m_gradients;
};

struct LevelData {
  Texture texture1;
  Texture texture2;
  std::optional<Texture> moving_mask;
  std::optional<Texture> fixed_mask;
  Calculator calculator;
  std::optional<Calculator> reverse_calculator;
};

struct ChannelData {
  std::vector<LevelData> levels;
  float weight = 1.0F;
};

RegistrationResult run_registration(const RegistrationConfig &config, const GPU::ComputeContext &context) {
  constexpr uint32_t num_levels = 3U;
  const bool is_affine = config.transformation_type == TransformationType::Affine;
  const uint32_t degrees_of_freedom = is_affine ? 12U : 6U;

  std::vector<ChannelData> channels_data;
  for (const auto &channel_config : config.channels) {
    const auto &image1 = channel_config.image1;
    const auto &image2 = channel_config.image2;

    const Texture texture1 = context.new_texture_from_host_image(image1);
    const Texture texture2 = context.new_texture_from_host_image(image2);
    std::optional<Texture> texture1_mask;
    if (channel_config.image1Mask) {
      texture1_mask = context.new_texture_from_host_image(*channel_config.image1Mask);
    }
    std::optional<Texture> texture2_mask;
    if (channel_config.image2Mask) {
      texture2_mask = context.new_texture_from_host_image(*channel_config.image2Mask);
    }

    const std::vector<Texture> pyramid1 = createDownsampledPyramid(texture1, num_levels, context);
    const std::vector<Texture> pyramid2 = createDownsampledPyramid(texture2, num_levels, context);
    std::vector<Texture> pyramid1_mask;
    if (texture1_mask) {
      pyramid1_mask = createDownsampledPyramid(*texture1_mask, num_levels, context);
    }
    std::vector<Texture> pyramid2_mask;
    if (texture2_mask) {
      pyramid2_mask = createDownsampledPyramid(*texture2_mask, num_levels, context);
    }

    std::vector<LevelData> levels;
    for (size_t level = 0; level < num_levels; ++level) {
      // The pyramid is arranged so index 0 is the lowest resolution and
      // index (num_levels-1) is full resolution. The transform downscale is
      // how much the texture is downsampled relative to the original image.
      const float level_downscale = std::exp2f(static_cast<float>(num_levels - 1U - level));

      std::optional<Texture> level_moving_mask;
      if (!pyramid1_mask.empty()) {
        level_moving_mask = pyramid1_mask[level];
      }
      std::optional<Texture> level_fixed_mask;
      if (!pyramid2_mask.empty()) {
        level_fixed_mask = pyramid2_mask[level];
      }

      auto calculator = MR::match_v(
          config.metric,
          [&](const NMIMetric &nmi_metric) -> Calculator {
            const NMICalculator::Config nmi_config{
                .transformation_type = config.transformation_type,
                .fixed = pyramid2[level],
                .moving = pyramid1[level],
                .fixed_mask = level_fixed_mask,
                .moving_mask = level_moving_mask,
                .voxel_scanner_matrices = VoxelScannerMatrices::from_image_pair(image1, image2, level_downscale),
                .num_bins = nmi_metric.num_bins,
                .output = CalculatorOutput::CostAndGradients,
                .context = &context,
            };
            return NMICalculator(nmi_config);
          },
          [&](const SSDMetric &) -> Calculator {
            const SSDCalculator::Config ssd_config{
                .transformation_type = config.transformation_type,
                .fixed = pyramid2[level],
                .moving = pyramid1[level],
                .fixed_mask = level_fixed_mask,
                .moving_mask = level_moving_mask,
                .voxel_scanner_matrices = VoxelScannerMatrices::from_image_pair(image1, image2, level_downscale),
                .output = CalculatorOutput::CostAndGradients,
                .context = &context,
            };
            return SSDCalculator(ssd_config);
          },
          [&](const NCCMetric &ncc_metric) -> Calculator {
            const NCCCalculator::Config ncc_config{
                .transformation_type = config.transformation_type,
                .fixed = pyramid2[level],
                .moving = pyramid1[level],
                .fixed_mask = level_fixed_mask,
                .moving_mask = level_moving_mask,
                .voxel_scanner_matrices = VoxelScannerMatrices::from_image_pair(image1, image2, level_downscale),
                .window_radius = ncc_metric.window_radius,
                .output = CalculatorOutput::CostAndGradients,
                .context = &context,
            };
            return NCCCalculator(ncc_config);
          });

      std::optional<Calculator> reverse_calculator;
      if (level == (num_levels - 1U)) {
        reverse_calculator = MR::match_v(
            config.metric,
            [&](const NMIMetric &nmi_metric) -> Calculator {
              const NMICalculator::Config nmi_config{
                  .transformation_type = config.transformation_type,
                  .fixed = pyramid1[level],
                  .moving = pyramid2[level],
                  .fixed_mask = level_moving_mask,
                  .moving_mask = level_fixed_mask,
                  .voxel_scanner_matrices =
                      VoxelScannerMatrices::from_image_pair(image2, image1, level_downscale),
                  .num_bins = nmi_metric.num_bins,
                  .output = CalculatorOutput::CostAndGradients,
                  .context = &context,
              };
              return NMICalculator(nmi_config);
            },
            [&](const SSDMetric &) -> Calculator {
              const SSDCalculator::Config ssd_config{
                  .transformation_type = config.transformation_type,
                  .fixed = pyramid1[level],
                  .moving = pyramid2[level],
                  .fixed_mask = level_moving_mask,
                  .moving_mask = level_fixed_mask,
                  .voxel_scanner_matrices =
                      VoxelScannerMatrices::from_image_pair(image2, image1, level_downscale),
                  .output = CalculatorOutput::CostAndGradients,
                  .context = &context,
              };
              return SSDCalculator(ssd_config);
            },
            [&](const NCCMetric &ncc_metric) -> Calculator {
              const NCCCalculator::Config ncc_config{
                  .transformation_type = config.transformation_type,
                  .fixed = pyramid1[level],
                  .moving = pyramid2[level],
                  .fixed_mask = level_moving_mask,
                  .moving_mask = level_fixed_mask,
                  .voxel_scanner_matrices =
                      VoxelScannerMatrices::from_image_pair(image2, image1, level_downscale),
                  .window_radius = ncc_metric.window_radius,
                  .output = CalculatorOutput::CostAndGradients,
                  .context = &context,
              };
              return NCCCalculator(ncc_config);
            });
      }
      levels.emplace_back(
          LevelData{pyramid1[level],
                    pyramid2[level],
                    level_moving_mask,
                    level_fixed_mask,
                    std::move(calculator),
                    std::move(reverse_calculator)});
    }
    channels_data.emplace_back(ChannelData{.levels = levels, .weight = channel_config.weight});
  }
  if (channels_data.empty()) {
    throw MR::Exception("No channels provided for registration");
  }

  const GlobalTransform initial_transform = [&]() {
    return MR::match_v(
        config.initial_guess,
        [&](const transform_type &t) {
          return GlobalTransform::from_affine_compact(
              t,
              image_centre_scanner_space<Image<float>, float>(config.channels.front().image1),
              config.transformation_type);
        },
        [&](const InitialisationOptions &init_options) {
          // Use the lowest resolution level for initialisation from the first channel only
          const auto &first_level = channels_data.front().levels.front();
          const float init_transform_downscale = std::exp2f(static_cast<float>(num_levels - 1U));
          const auto voxel_scanner = VoxelScannerMatrices::from_image_pair(
              config.channels.front().image1, config.channels.front().image2, init_transform_downscale);

          const InitialisationConfig init_config{
              .moving_texture = first_level.texture1,
              .target_texture = first_level.texture2,
              .moving_mask = first_level.moving_mask,
              .target_mask = first_level.fixed_mask,
              .voxel_scanner_matrices = voxel_scanner,
              .options = init_options,
          };

          const auto rigid = initialise_transformation(init_config, context);

          return config.transformation_type == TransformationType::Rigid ? rigid.as_rigid() : rigid.as_affine();
        });
  }();

  const auto convergence_thresholds = make_convergence_thresholds(degrees_of_freedom);

  GlobalTransform current_transform = initial_transform;
  GlobalTransform best_transform = initial_transform;

  // To make registration symmetric we run the registration in both directions
  // and take the average of the resulting transforms at each iteration using Lie algebra averaging.
  // This avoids the need of defining an average middle space which can introduce sampling bias.
  // See https://doi.org/10.1117/1.jmi.1.2.024003 by Modat et al.
  // This only needs to be be done for the final level since lower levels are just approximations.
  // TODO: verify the symmetricity of the registration process.
  for (size_t level = 0; level < num_levels; ++level) {
    GlobalTransform best_transform_level = current_transform;
    float best_cost = std::numeric_limits<float>::infinity();
    ConvergenceChecker convergence_checker(ConvergenceChecker::Config{
        .patienceLimit = (level == 0U) ? coarsest_level_patience : finer_levels_patience,
        .param_thresholds = convergence_thresholds,
    });
    const float learning_rate = base_learning_rate / std::pow(2.0F, static_cast<float>(level));

    const bool symmetric_level = (level == (num_levels - 1U));
    if (!symmetric_level) {
      AdaBelief adabelief = create_optimiser(current_transform.parameters(), learning_rate);

      for (size_t iter = 0; iter < config.max_iterations; ++iter) {
        // Dispatch gradient calculations for all channels
        for (auto &channel_data : channels_data) {
          auto &channel_calculator = channel_data.levels[level].calculator;
          channel_calculator.update(current_transform);
        }

        WeightedGradients channel_gradients(degrees_of_freedom);
        float total_cost = 0.0F;
        // Gather results for each channel accumulating gradients and cost
        for (const auto &channel_data : channels_data) {
          const auto &channel_calculator = channel_data.levels[level].calculator;
          const auto channel_result = channel_calculator.get_result();
          channel_gradients.add(channel_result.gradients, channel_data.weight);
          total_cost += channel_result.cost * channel_data.weight;
        }

        if (total_cost < best_cost) {
          best_cost = total_cost;
          best_transform_level = current_transform;
        }

        INFO("Current transformation matrix at level " + std::to_string(level) + " iteration " +
             std::to_string(iter) + ":\n" + EigenHelpers::to_string(current_transform.to_matrix4f()));

        INFO("Level " + std::to_string(level) + ", Iteration " + std::to_string(iter) +
             ", Cost: " + std::to_string(total_cost));

        if (convergence_checker.has_converged(current_transform.parameters(), total_cost)) {
          CONSOLE("Convergence reached at level " + std::to_string(level) + " after " + std::to_string(iter) +
                  " iterations.");
          break;
        }
        adabelief.step(channel_gradients.get());
        const auto updated_params = adabelief.parameterValues();
        current_transform.set_params(updated_params);
      }
      current_transform = best_transform_level;
      best_transform = best_transform_level;
      continue;
    }

    const auto pivot_moving =
        image_centre_scanner_space<Image<float>, float>(config.channels.front().image1).template cast<float>();
    const auto pivot_fixed =
        image_centre_scanner_space<Image<float>, float>(config.channels.front().image2).template cast<float>();

    // Re-parameterise the current transform around the fixed pivot for the forward direction.
    GlobalTransform current_transform_fwd = GlobalTransform::from_affine_compact(
        current_transform.to_affine_compact(), pivot_fixed, config.transformation_type);
    GlobalTransform current_transform_bwd = GlobalTransform::from_affine_compact(
        current_transform.to_affine_compact().inverse(), pivot_moving, config.transformation_type);

    AdaBelief adabelief_fwd = create_optimiser(current_transform_fwd.parameters(), learning_rate);
    AdaBelief adabelief_bwd = create_optimiser(current_transform_bwd.parameters(), learning_rate);

    for (size_t iter = 0; iter < config.max_iterations; ++iter) {
      // Dispatch gradient calculations for all channels in both directions.
      for (auto &channel_data : channels_data) {
        auto &channel_calculator_fwd = channel_data.levels[level].calculator;
        channel_calculator_fwd.update(current_transform_fwd);

        auto &reverse_calculator = channel_data.levels[level].reverse_calculator;
        reverse_calculator->update(current_transform_bwd);
      }

      WeightedGradients channel_gradients_fwd(degrees_of_freedom);
      WeightedGradients channel_gradients_bwd(degrees_of_freedom);
      float total_cost_fwd = 0.0F;
      float total_cost_bwd = 0.0F;

      // Gather results for each channel accumulating gradients and cost.
      for (const auto &channel_data : channels_data) {
        const auto &channel_calculator_fwd = channel_data.levels[level].calculator;
        const auto channel_result_fwd = channel_calculator_fwd.get_result();
        channel_gradients_fwd.add(channel_result_fwd.gradients, channel_data.weight);
        total_cost_fwd += channel_result_fwd.cost * channel_data.weight;

        const auto &reverse_calculator = channel_data.levels[level].reverse_calculator;
        const auto channel_result_bwd = reverse_calculator->get_result();
        channel_gradients_bwd.add(channel_result_bwd.gradients, channel_data.weight);
        total_cost_bwd += channel_result_bwd.cost * channel_data.weight;
      }

      const float total_cost = total_cost_fwd + total_cost_bwd;
      if (total_cost < best_cost) {
        best_cost = total_cost;
        best_transform_level = current_transform_fwd;
      }

      INFO("Current transformation matrix (fwd) at level " + std::to_string(level) + " iteration " +
           std::to_string(iter) + ":\n" + EigenHelpers::to_string(current_transform_fwd.to_matrix4f()));
      INFO("Current transformation matrix (bwd) at level " + std::to_string(level) + " iteration " +
           std::to_string(iter) + ":\n" + EigenHelpers::to_string(current_transform_bwd.to_matrix4f()));

      INFO("Level " + std::to_string(level) + ", Iteration " + std::to_string(iter) +
           ", Cost (fwd+bwd): " + std::to_string(total_cost_fwd) + "+" + std::to_string(total_cost_bwd) +
           " = " + std::to_string(total_cost));

      if (convergence_checker.has_converged(current_transform_fwd.parameters(), total_cost)) {
        CONSOLE("Convergence reached at level " + std::to_string(level) + " after " +
                std::to_string(iter) + " iterations.");
        break;
      }

      adabelief_fwd.step(channel_gradients_fwd.get());
      adabelief_bwd.step(channel_gradients_bwd.get());

      const auto updated_params_fwd = adabelief_fwd.parameterValues();
      const auto updated_params_bwd = adabelief_bwd.parameterValues();
      current_transform_fwd.set_params(updated_params_fwd);
      current_transform_bwd.set_params(updated_params_bwd);

      // Lie algebra averaging to enforce symmetry.
      const Eigen::Matrix4f t_fwd = current_transform_fwd.to_matrix4f();
      const Eigen::Matrix4f t_bwd = current_transform_bwd.to_matrix4f();
      const Eigen::Matrix4f mean_log = 0.5F * (t_fwd.log() + t_bwd.inverse().log());
      const Eigen::Matrix4f avg_mat = mean_log.exp();
      const auto avg_tform = EigenHelpers::from_homogeneous_mat4f(avg_mat);

      current_transform_fwd = GlobalTransform::from_affine_compact(avg_tform, pivot_fixed, config.transformation_type);
      current_transform_bwd =
          GlobalTransform::from_affine_compact(avg_tform.inverse(), pivot_moving, config.transformation_type);

      const auto params_fwd = current_transform_fwd.parameters();
      adabelief_fwd.setParameterValues(params_fwd);
      const auto params_bwd = current_transform_bwd.parameters();
      adabelief_bwd.setParameterValues(params_bwd);
    }

    current_transform = best_transform_level;
    best_transform = best_transform_level;
  }

  INFO("Final transformation matrix:\n" + EigenHelpers::to_string(best_transform.to_matrix4f()));
  return RegistrationResult{.transformation = best_transform.to_affine_compact()};
}

} // namespace MR::GPU
