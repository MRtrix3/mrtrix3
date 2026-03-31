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

#include "gpu/registration/initialisation.h"
#include "gpu/gpu.h"
#include "gpu/registration/calculatorinterface.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/registration/imageoperations.h"
#include "gpu/registration/initialisation_rotation_search.h"
#include "gpu/registration/ncccalculator.h"
#include "gpu/registration/nmicalculator.h"
#include "gpu/registration/registrationtypes.h"
#include "gpu/registration/ssdcalculator.h"
#include "match_variant.h"
#include "math/math.h"
#include "mrtrix.h"
#include "types.h"
#include <tcb/span.hpp>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using vec3 = std::array<float, 3>;

namespace {

// Returns `num_samples` axis-angle vectors stored as {x, y, z} where the vector direction
// is the rotation axis (unit length).
// See https://stackoverflow.com/questions/9600801/evenly-distributing-n-points-on-a-sphere
std::vector<vec3> fibonacci_sphere_axes(int32_t num_samples) {
  std::vector<vec3> out;
  if (num_samples <= 0) {
    throw std::invalid_argument("num_samples must be positive");
  }

  const int32_t n = num_samples;

  const double pi = std::acos(-1.0);
  const double golden_angle = pi * (3.0 - std::sqrt(5.0)); // ~= 2.399963229728653

  out.reserve(n);

  for (int32_t i = 0; i < n; ++i) {
    // y in [-1, 1]. If n == 1, place at north pole (y = 1).
    const double y = (n == 1) ? 1.0 : 1.0 - ((2.0 * i) / static_cast<double>(n - 1));
    const double radius = std::sqrt(std::max(0.0, 1.0 - (y * y)));
    const double phi = i * golden_angle;

    const double x = std::cos(phi) * radius;
    const double z = std::sin(phi) * radius;

    const std::array axis = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    out.push_back(axis);
  }

  return out;
}

std::vector<float> evenly_spaced_angles(int32_t num_shells, float min_angle, float max_angle) {
  std::vector<float> out;
  if (num_shells <= 0) {
    throw std::invalid_argument("num_shells must be positive");
  }

  if (min_angle > max_angle) {
    std::swap(min_angle, max_angle);
  }

  out.reserve(static_cast<size_t>(num_shells));
  if (num_shells == 1) {
    out.push_back(max_angle);
    return out;
  }

  for (int32_t shell_index = 0; shell_index < num_shells; ++shell_index) {
    const float t = static_cast<float>(shell_index) / static_cast<float>(num_shells - 1);
    out.push_back(min_angle + (t * (max_angle - min_angle)));
  }

  return out;
}

std::vector<vec3>
axis_angle_grid_samples(int32_t num_axes, int32_t num_angle_shells, float min_angle, float max_angle) {
  if (num_axes <= 0) {
    throw std::invalid_argument("num_axes must be positive");
  }

  const auto axes = fibonacci_sphere_axes(num_axes);
  const auto angles = evenly_spaced_angles(num_angle_shells, min_angle, max_angle);

  std::vector<vec3> out;
  out.reserve(1U + (axes.size() * angles.size()));

  const bool include_identity = min_angle <= std::numeric_limits<float>::epsilon();
  if (include_identity) {
    out.push_back({0.0F, 0.0F, 0.0F});
  }

  for (const float angle : angles) {
    if (include_identity && angle <= std::numeric_limits<float>::epsilon()) {
      continue;
    }

    for (const auto &axis : axes) {
      out.push_back({axis[0] * angle, axis[1] * angle, axis[2] * angle});
    }
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

void make_right_handed(Eigen::Matrix3f &basis) {
  if (basis.determinant() < 0.0F) {
    basis.col(2) *= -1.0F;
  }
}

Eigen::Matrix3f best_moment_rotation(Eigen::Matrix3f target_basis, Eigen::Matrix3f moving_basis) {
  make_right_handed(target_basis);
  make_right_handed(moving_basis);

  const std::array<Eigen::Vector3f, 4> sign_patterns = {
      Eigen::Vector3f(1.0F, 1.0F, 1.0F),
      Eigen::Vector3f(1.0F, -1.0F, -1.0F),
      Eigen::Vector3f(-1.0F, 1.0F, -1.0F),
      Eigen::Vector3f(-1.0F, -1.0F, 1.0F),
  };

  float best_trace = -1.0F;
  Eigen::Matrix3f best_rotation = Eigen::Matrix3f::Identity();

  for (const auto &signs : sign_patterns) {
    Eigen::Matrix3f signed_moving_basis = moving_basis;
    for (int32_t axis = 0; axis < 3; ++axis) {
      signed_moving_basis.col(axis) *= signs[axis];
    }

    const Eigen::Matrix3f rotation = signed_moving_basis * target_basis.transpose();
    const float trace = rotation.trace();
    if (trace > best_trace) {
      best_trace = trace;
      best_rotation = rotation;
    }
  }

  return best_rotation;
}

Eigen::Vector3f geometric_centre_scanner_space(const MR::GPU::Texture &texture,
                                               const Eigen::Matrix4f &voxel_to_scanner) {
  const Eigen::Vector4f centre_voxel((static_cast<float>(texture.spec.width) - 1.0F) * 0.5F,
                                     (static_cast<float>(texture.spec.height) - 1.0F) * 0.5F,
                                     (static_cast<float>(texture.spec.depth) - 1.0F) * 0.5F,
                                     1.0F);
  return (voxel_to_scanner * centre_voxel).head<3>();
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

  const auto com_target =
      EigenHelpers::to_vector3f(centerOfMass(target_texture, context, transform_type::Identity(), target_mask));
  const Eigen::Map<const Eigen::Matrix4f> voxel_to_scanner_fixed(voxel_scanner_matrices.voxel_to_scanner_fixed.data());
  const Eigen::Map<const Eigen::Matrix4f> voxel_to_scanner_moving(
      voxel_scanner_matrices.voxel_to_scanner_moving.data());
  const Eigen::Vector4f com_target_scanner = voxel_to_scanner_fixed * com_target.homogeneous();
  const Eigen::Vector3f moving_geometric_centre_scanner =
      geometric_centre_scanner_space(moving_texture, voxel_to_scanner_moving);
  Eigen::Vector3f rotation_search_anchor_scanner = moving_geometric_centre_scanner;

  const std::array<float, 6> rigid_identity = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  GlobalTransform initial_transform(rigid_identity, GlobalRegistrationType::Rigid, com_target_scanner.head<3>());

  switch (options.translation_choice) {
  case InitTranslationChoice::None:
    break;
  case MR::InitTranslationChoice::Mass: {
    INFO("Computing initial translation using center of mass.");
    const auto com_moving =
        EigenHelpers::to_vector3f(centerOfMass(moving_texture, context, transform_type::Identity(), moving_mask));
    const Eigen::Vector4f com_moving_scanner = voxel_to_scanner_moving * com_moving.homogeneous();

    initial_transform.set_translation(com_moving_scanner.head<3>() - com_target_scanner.head<3>());
    rotation_search_anchor_scanner = com_moving_scanner.head<3>();
    break;
  }
  case InitTranslationChoice::Geometric: {
    INFO("Computing initial translation using geometric center.");
    const Eigen::Vector3f geom_moving_scanner = moving_geometric_centre_scanner;
    const Eigen::Vector3f geom_target_scanner = geometric_centre_scanner_space(target_texture, voxel_to_scanner_fixed);

    initial_transform.set_translation(geom_moving_scanner - geom_target_scanner);
    initial_transform.set_pivot(geom_target_scanner);
    rotation_search_anchor_scanner = geom_moving_scanner;
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
                .transformation_type = GlobalRegistrationType::Rigid,
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
                .transformation_type = GlobalRegistrationType::Rigid,
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
                .transformation_type = GlobalRegistrationType::Rigid,
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
    constexpr int32_t coarse_rotation_search_axis_count = 96;
    constexpr int32_t coarse_rotation_search_angle_shell_count = 25;
    const auto samples = axis_angle_grid_samples(
        coarse_rotation_search_axis_count, coarse_rotation_search_angle_shell_count, 0.0F, max_angle_rad);

    INFO("max_search_angle_degrees=" + std::to_string(options.max_search_angle_degrees) +
         " max_angle_rad=" + std::to_string(max_angle_rad));
    INFO("rotation search sampling " + std::to_string(coarse_rotation_search_axis_count) + " axes across " +
         std::to_string(coarse_rotation_search_angle_shell_count) + " angle shells (" + std::to_string(samples.size()) +
         " total axis-angle candidates)");

    const auto make_rotation_calculator = [&]() -> RotationSearchCalculator {
      auto calculator = std::make_shared<Calculator>(make_calculator());
      return RotationSearchCalculator{
          .update = [calculator](const GlobalTransform &transform) { calculator->update(transform); },
          .get_result = [calculator]() { return calculator->get_result(); },
      };
    };

    const Eigen::Vector3f search_anchor_scanner = rotation_search_anchor_scanner;
    const Eigen::Vector3f target_anchor_scanner =
        (initial_transform.to_affine_compact() * search_anchor_scanner.cast<double>()).cast<float>();
    const auto build_search_transform = [&](const std::array<float, 3> &sample) {
      std::array<float, 6> candidate_params{};
      candidate_params[3] = sample[0];
      candidate_params[4] = sample[1];
      candidate_params[5] = sample[2];

      GlobalTransform candidate_transform(tcb::span<const float>(candidate_params.data(), candidate_params.size()),
                                          initial_transform.type(),
                                          initial_transform.pivot());
      const Eigen::Vector3f rotated_anchor_scanner =
          (candidate_transform.to_affine_compact() * search_anchor_scanner.cast<double>()).cast<float>();
      candidate_transform.set_translation(target_anchor_scanner - rotated_anchor_scanner);
      return candidate_transform;
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
                             build_search_transform,
                             search_params,
                             [&](float best_cost, const std::array<float, 3> &best_rotation) {
                               INFO("New best initial rotation found with cost " + std::to_string(best_cost) +
                                    " at axis-angle {" + std::to_string(best_rotation[0]) + ", " +
                                    std::to_string(best_rotation[1]) + ", " + std::to_string(best_rotation[2]) + "}");
                             });
    initial_transform = build_search_transform({best_rotation[0], best_rotation[1], best_rotation[2]});
    break;
  }
  case InitRotationChoice::Moments: {
    INFO("Computing initial rotation using image moments.");

    const auto com_moving =
        EigenHelpers::to_vector3f(centerOfMass(moving_texture, context, transform_type::Identity(), moving_mask));
    const Eigen::Vector4f com_moving_scanner = voxel_to_scanner_moving * com_moving.homogeneous();

    const Eigen::Matrix3f target_moments = computeScannerMoments(
        target_texture, context, voxel_to_scanner_fixed, com_target_scanner.head<3>(), target_mask);
    const Eigen::Matrix3f moving_moments = computeScannerMoments(
        moving_texture, context, voxel_to_scanner_moving, com_moving_scanner.head<3>(), moving_mask);

    Eigen::Matrix3f target_basis;
    Eigen::Matrix3f moving_basis;
    Eigen::Vector3f target_eigenvalues;
    Eigen::Vector3f moving_eigenvalues;
    if (!compute_sorted_eigenvectors(target_moments, target_basis, target_eigenvalues) ||
        !compute_sorted_eigenvectors(moving_moments, moving_basis, moving_eigenvalues)) {
      throw std::runtime_error("Failed to compute principal axes for moment-based initial rotation.");
    }

    const Eigen::Matrix3f rotation_matrix = best_moment_rotation(target_basis, moving_basis);
    const Eigen::AngleAxisf angle_axis(rotation_matrix);
    initial_transform.set_rotation(angle_axis.axis() * angle_axis.angle());
    break;
  }
  }

  INFO("Initial transformation matrix:\n" + EigenHelpers::to_string(initial_transform.to_matrix4f()));

  return initial_transform.as_affine();
}
} // namespace MR::GPU
