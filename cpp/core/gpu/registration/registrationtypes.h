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

#include "image.h"
#include <tcb/span.hpp>
#include "types.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace MR {

enum class TransformationType : uint8_t { Rigid, Affine };

// The parameters in order:
// - 3 translations
// - 3 rotations (axis-angle representation)
// - 3 scaling factors
// - 3 shearing factors
// Order of application: shear, scale, rotate, translates.
// All operations are assumed to be applied by taking the pivot
// point as the centre of the transformation.
struct GlobalTransform {

  // Throws if params.size() does not match type
  explicit GlobalTransform(tcb::span<const float> params,
                           TransformationType type,
                           const Eigen::Vector3f &pivot = Eigen::Vector3f::Zero());

  GlobalTransform inverse() const;
  // Obtain a copy with a different pivot
  GlobalTransform with_pivot(const Eigen::Vector3f &pivot) const;

  // Create GlobalTransform from an Eigen transform and pivot.
  // If type is specified as rigid, scale and shear components are ignored.
  static GlobalTransform from_affine_compact(const transform_type &tform,
                                             const Eigen::Vector3f &pivot,
                                             TransformationType type = TransformationType::Affine);

  // Returns a copy that keeps translation and axis-angle rotation, dropping any scale/shear terms.
  GlobalTransform as_rigid() const;
  // Returns a copy that includes all affine params with rigid inputs being identity scale and
  // zero shear appended.
  GlobalTransform as_affine() const;

  TransformationType type() const;

  tcb::span<const float> parameters() const;
  void set_params(tcb::span<const float> params);

  Eigen::Vector3f pivot() const;
  void set_pivot(const Eigen::Vector3f &pivot);

  // Obtain a 3x4 Eigen affine-compact transform
  transform_type to_affine_compact() const;
  Eigen::Matrix4f to_matrix4f() const;

  bool is_rigid() const;
  bool is_affine() const;
  size_t param_count() const;

  Eigen::Vector3f translation() const;
  void set_translation(const Eigen::Vector3f &translation);

  void set_rotation(const Eigen::Vector3f &rotation_axis_angle);
  Eigen::Vector3f rotation() const;

  // For rigid case, scale defaults to (1,1,1) and shears to (0,0,0).
  // Setters throw if called on rigid transforms.
  Eigen::Vector3f scale() const;
  void set_scale(const Eigen::Vector3f &scale);

  Eigen::Vector3f shear() const;
  void set_shear(const Eigen::Vector3f &shear);

private:
  TransformationType m_type;
  // We allocate space for the maximum number of parameters.
  std::array<float, 12> m_params{};
  size_t m_param_count = 0U;
  Eigen::Vector3f m_pivot;
};

struct IterationResult {
  float cost;
  std::vector<float> gradients;
};

struct NMIMetric {
  uint32_t num_bins = 32;
};

struct SSDMetric {};

struct NCCMetric {
  uint32_t window_radius = 0U;
};

using Metric = std::variant<NMIMetric, SSDMetric, NCCMetric>;

enum class MetricType : uint8_t { NMI, SSD, NCC };
enum class InitTranslationChoice : uint8_t { None, Mass, Geometric };
enum class InitRotationChoice : uint8_t { None, Search, Moments };

struct InitialisationOptions {
  InitTranslationChoice translation_choice = InitTranslationChoice::Mass;
  InitRotationChoice rotation_choice = InitRotationChoice::None;
  Metric cost_metric = NMIMetric{};
  // Limits the maximum sampled rotation angle (degrees) for search-based initialisation.
  float max_search_angle_degrees = 90.0F;
};

using InitialGuess = std::variant<transform_type, InitialisationOptions>;

struct ChannelConfig {
  Image<float> image1;
  Image<float> image2;
  std::optional<Image<float>> image1Mask;
  std::optional<Image<float>> image2Mask;
  float weight = 1.0F;
};

struct RegistrationConfig {
  std::vector<ChannelConfig> channels;
  TransformationType transformation_type;
  InitialGuess initial_guess;
  Metric metric;
  uint32_t max_iterations = 500;
};

struct RegistrationResult {
  transform_type transformation;
};

} // namespace MR
