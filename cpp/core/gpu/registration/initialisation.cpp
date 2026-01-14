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

#include "gpu/registration/initialisation.h"
#include "gpu/registration/calculatorinterface.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/gpu.h"
#include "gpu/registration/imageoperations.h"
#include "gpu/registration/initialisation_rotation_search.h"
#include "match_variant.h"
#include "math/math.h"
#include "mrtrix.h"
#include "gpu/registration/ncccalculator.h"
#include "gpu/registration/nmicalculator.h"
#include "gpu/registration/registrationtypes.h"
#include <tcb/span.hpp>
#include "gpu/registration/ssdcalculator.h"
#include "types.h"

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <vector>

using vec3 = std::array<float, 3>;

namespace {

// Returns `num_samples` axis-angle vectors stored as {x, y, z} where the vector direction
// is the rotation axis (unit length) and the vector magnitude is the rotation angle theta.
// Angles are in radians.
// See https://stackoverflow.com/questions/9600801/evenly-distributing-n-points-on-a-sphere
std::vector<vec3> fibonacci_sphere_samples(int32_t num_samples, float min_angle, float max_angle) {
  std::vector<vec3> out;
  if (num_samples <= 0) {
    throw std::invalid_argument("num_samples must be positive");
  }

  if (min_angle > max_angle)
    std::swap(min_angle, max_angle);

  const int32_t n = num_samples;

  const double pi = std::acos(-1.0);
  const double golden_angle = pi * (3.0 - std::sqrt(5.0)); // ~= 2.399963229728653

  out.reserve(n);

  for (int i = 0; i < n; ++i) {
    // y in [-1, 1]. If n == 1, place at north pole (y = 1).
    const double y = (n == 1) ? 1.0 : 1.0 - ((2.0 * i) / static_cast<double>(n - 1));
    const double radius = std::sqrt(std::max(0.0, 1.0 - (y * y)));
    const double phi = i * golden_angle;

    const double x = std::cos(phi) * radius;
    const double z = std::sin(phi) * radius;

    // compute angle for this sample using lerp in [min_angle, max_angle]
    const double t = (n == 1) ? 0.5 : (static_cast<double>(i) / static_cast<double>(n - 1));
    const double angle =
        static_cast<double>(min_angle) + (t * (static_cast<double>(max_angle) - static_cast<double>(min_angle)));

    // axis-angle vector = unit_axis * angle
    const std::array axis = {
        static_cast<float>(x * angle), static_cast<float>(y * angle), static_cast<float>(z * angle)};
    out.push_back(axis);
  }

  return out;
}

bool compute_sorted_eigenvectors(const Eigen::Matrix3f &matrix,
                                 Eigen::Matrix3f &eigenvectors,
                                 Eigen::Vector3f &eigenvalues) {
  if (!matrix.allFinite()) {
    return false;
  }

  const Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(matrix);
  if (solver.info() != Eigen::Success) {
    return false;
  }

  const Eigen::Vector3f values = solver.eigenvalues();
  const Eigen::Matrix3f vectors = solver.eigenvectors();

  std::array<int, 3> indices = {0, 1, 2};
  std::sort(indices.begin(), indices.end(), [&](int a, int b) { return values[a] > values[b]; });

  for (size_t i = 0; i < indices.size(); ++i) {
    eigenvalues[static_cast<int>(i)] = values[indices[i]];
    eigenvectors.col(static_cast<int>(i)) = vectors.col(indices[i]);
  }
  return eigenvectors.allFinite() && eigenvalues.allFinite();
}
} // namespace

namespace MR::GPU {
GlobalTransform initialise_transformation(const InitialisationConfig &config, const ComputeContext &context) {
  const Texture &moving_texture = config.moving_texture;
  const Texture &target_texture = config.target_texture;
  const auto &voxel_scanner_matrices = config.voxel_scanner_matrices;
  const auto &moving_mask = config.moving_mask;
  const auto &target_mask = config.target_mask;
  const InitialisationOptions &options = config.options;

  const auto com_target = EigenHelpers::to_vector3f(
      centerOfMass(target_texture, context, transform_type::Identity(), target_mask));
  const Eigen::Map<const Eigen::Matrix4f> voxel_to_scanner_fixed(voxel_scanner_matrices.voxel_to_scanner_fixed.data());
  const Eigen::Vector4f com_target_scanner = voxel_to_scanner_fixed * com_target.homogeneous();

  const std::array<float, 6> rigid_identity = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  GlobalTransform initial_transform(rigid_identity, TransformationType::Rigid, com_target_scanner.head<3>());

  switch (options.translation_choice) {
  case InitTranslationChoice::None:
    break;
  case MR::InitTranslationChoice::Mass: {
    INFO("Computing initial translation using center of mass.");
    const auto com_moving = EigenHelpers::to_vector3f(
        centerOfMass(moving_texture, context, transform_type::Identity(), moving_mask));
    const Eigen::Map<const Eigen::Matrix4f> voxel_to_scanner_moving(
        voxel_scanner_matrices.voxel_to_scanner_moving.data());
    const Eigen::Vector4f com_moving_scanner = voxel_to_scanner_moving * com_moving.homogeneous();

    initial_transform.set_translation(com_moving_scanner.head<3>() - com_target_scanner.head<3>());
    break;
  }
  case InitTranslationChoice::Geometric: {
    INFO("Computing initial translation using geometric center.");
    const Eigen::Vector4f geom_moving_voxel((static_cast<float>(moving_texture.spec.width) - 1.0F) * 0.5F,
                                            (static_cast<float>(moving_texture.spec.height) - 1.0F) * 0.5F,
                                            (static_cast<float>(moving_texture.spec.depth) - 1.0F) * 0.5F,
                                            1.0F);
    const Eigen::Vector4f geom_target_voxel((static_cast<float>(target_texture.spec.width) - 1.0F) * 0.5F,
                                            (static_cast<float>(target_texture.spec.height) - 1.0F) * 0.5F,
                                            (static_cast<float>(target_texture.spec.depth) - 1.0F) * 0.5F,
                                            1.0F);

    const Eigen::Map<const Eigen::Matrix4f> voxel_scanner_fixed(voxel_scanner_matrices.voxel_to_scanner_fixed.data());
    const Eigen::Map<const Eigen::Matrix4f> voxel_scanner_moving(voxel_scanner_matrices.voxel_to_scanner_moving.data());
    const Eigen::Vector4f geom_moving_scanner = voxel_scanner_moving * geom_moving_voxel;
    const Eigen::Vector4f geom_target_scanner = voxel_scanner_fixed * geom_target_voxel;

    initial_transform.set_translation(geom_moving_scanner.head<3>() - geom_target_scanner.head<3>());
    initial_transform.set_pivot(Eigen::Vector3f(geom_target_scanner.head<3>()));
    break;
  }
  }

  switch (options.rotation_choice) {
  case MR::InitRotationChoice::None:
    break;
  case MR::InitRotationChoice::Search: {
    INFO("Computing initial rotation using spherical sampling.");
    const auto make_calculator = [&]() -> Calculator {
      return MR::match_v(
          options.cost_metric,
          [&](const NMIMetric &nmi_metric) -> Calculator {
            const std::optional<Texture> fixed_mask = target_mask;
            const std::optional<Texture> moving_mask_opt = moving_mask;
            const NMICalculator::Config nmi_config{
                .transformation_type = TransformationType::Rigid,
                .fixed = target_texture,
                .moving = moving_texture,
                .fixed_mask = fixed_mask,
                .moving_mask = moving_mask_opt,
                .voxel_scanner_matrices = voxel_scanner_matrices,
                .num_bins = nmi_metric.num_bins,
                .output = CalculatorOutput::Cost,
                .context = &context,
            };
            return NMICalculator(nmi_config);
          },
          [&](const SSDMetric &) -> Calculator {
            const std::optional<Texture> fixed_mask = target_mask;
            const std::optional<Texture> moving_mask_opt = moving_mask;
            const SSDCalculator::Config ssd_config{
                .transformation_type = TransformationType::Rigid,
                .fixed = target_texture,
                .moving = moving_texture,
                .fixed_mask = fixed_mask,
                .moving_mask = moving_mask_opt,
                .voxel_scanner_matrices = voxel_scanner_matrices,
                .output = CalculatorOutput::Cost,
                .context = &context,
            };
            return SSDCalculator(ssd_config);
          },
          [&](const NCCMetric &ncc_metric) -> Calculator {
            const std::optional<Texture> fixed_mask = target_mask;
            const std::optional<Texture> moving_mask_opt = moving_mask;
            const NCCCalculator::Config ncc_config{
                .transformation_type = TransformationType::Rigid,
                .fixed = target_texture,
                .moving = moving_texture,
                .fixed_mask = fixed_mask,
                .moving_mask = moving_mask_opt,
                .voxel_scanner_matrices = voxel_scanner_matrices,
                .window_radius = ncc_metric.window_radius,
                .output = CalculatorOutput::Cost,
                .context = &context,
            };
            return NCCCalculator(ncc_config);
          });
    };

    constexpr float pi = MR::Math::pi;
    const float max_angle_rad = std::clamp(options.max_search_angle_degrees, 0.0F, 180.0F) * (pi / 180.0F);
    const auto samples = fibonacci_sphere_samples(500, 0.0F, max_angle_rad);

    const auto rotation_angle = [](const std::array<float, 3> &axis) {
      return std::sqrt(std::inner_product(axis.cbegin(), axis.cend(), axis.cbegin(), 0.0F));
    };

    INFO("max_search_angle_degrees=" + std::to_string(options.max_search_angle_degrees) +
         " max_angle_rad=" + std::to_string(max_angle_rad));
    INFO("sample[0] norm=" + std::to_string(rotation_angle(samples[0])) +
         " sample[last] norm=" + std::to_string(rotation_angle(samples.back())));

    const auto make_rotation_calculator = [&]() -> RotationSearchCalculator {
      auto calculator = std::make_shared<Calculator>(make_calculator());
      return RotationSearchCalculator{
          .update = [calculator](const GlobalTransform &transform) { calculator->update(transform); },
          .get_result = [calculator]() { return calculator->get_result(); },
      };
    };

    const RotationSearchParams search_params{
        .parallel_calculators = 8,
        .min_improvement = 1e-6F,
        .tie_cost_eps = 1e-6F,
    };
    const tcb::span<const std::array<float, 3>> sample_span(samples.data(), samples.size());
    const auto best_rotation =
        search_best_rotation(initial_transform,
                             sample_span,
                             make_rotation_calculator,
                             search_params,
                             [&](float best_cost, const std::array<float, 3> &best_rotation) {
                               INFO("New best initial rotation found with cost " + std::to_string(best_cost) +
                                    " at axis-angle {" + std::to_string(best_rotation[0]) + ", " +
                                    std::to_string(best_rotation[1]) + ", " + std::to_string(best_rotation[2]) + "}");
                             });

    std::array<float, 12> params{};
    const auto current_params = initial_transform.parameters();
    std::copy(current_params.begin(), current_params.end(), params.begin());
    params[3] = best_rotation[0];
    params[4] = best_rotation[1];
    params[5] = best_rotation[2];
    initial_transform = GlobalTransform(tcb::span<const float>(params.data(), initial_transform.param_count()),
                                        initial_transform.type(),
                                        initial_transform.pivot());
    break;
  }
  case InitRotationChoice::Moments: {
    // TODO: implement moment-based initial rotation
    throw std::logic_error("Moment-based initial rotation is not yet implemented.");
  }
  }

  INFO("Initial transformation matrix:\n" + EigenHelpers::to_string(initial_transform.to_matrix4f()));

  return initial_transform.as_affine();
}
} // namespace MR::GPU
