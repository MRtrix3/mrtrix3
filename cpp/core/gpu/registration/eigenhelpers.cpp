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

#include "gpu/registration/eigenhelpers.h"
#include "types.h"

#include <array>
#include <sstream>
#include <string>

namespace MR::EigenHelpers {

namespace {
template <typename Scalar> Eigen::Matrix<Scalar, 4, 4> to_homogeneous_mat4(const transform_type &source_transform) {
  Eigen::Matrix<Scalar, 4, 4> matrix = Eigen::Matrix<Scalar, 4, 4>::Identity();
  matrix.template block<3, 4>(0, 0) = source_transform.matrix().template cast<Scalar>();
  return matrix;
}

template <typename Scalar> transform_type from_homogeneous_mat4(const Eigen::Matrix<Scalar, 4, 4> &source_matrix) {
  transform_type result;
  result.linear() = source_matrix.template block<3, 3>(0, 0).template cast<default_type>();
  result.translation() = source_matrix.template block<3, 1>(0, 3).template cast<default_type>();
  return result;
}
} // namespace

// Eigen::Transform<default_type, 3, Eigen::AffineCompact>
Eigen::Matrix4f to_homogeneous_mat4f(const transform_type &source_transform) {
  return to_homogeneous_mat4<float>(source_transform);
}

Eigen::Matrix4d to_homogeneous_mat4d(const transform_type &source_transform) {
  return to_homogeneous_mat4<double>(source_transform);
}

transform_type from_homogeneous_mat4f(const Eigen::Matrix4f &source_matrix) {
  return from_homogeneous_mat4<float>(source_matrix);
}

transform_type from_homogeneous_mat4d(const Eigen::Matrix4d &source_matrix) {
  return from_homogeneous_mat4<double>(source_matrix);
}

std::array<float, 16> to_array(const Eigen::Matrix4f &matrix) {
  std::array<float, 16> array{};
  Eigen::Map<Eigen::Matrix4f>(array.data()) = matrix;
  return array;
}

std::array<float, 16> to_array(const transform_type &transform) {
  return to_array(to_homogeneous_mat4f(transform));
}

Eigen::Vector3f to_vector3f(const std::array<float, 3> &array) {
  return Eigen::Vector3f(array[0], array[1], array[2]);
}

std::array<float, 3> to_array(const Eigen::Vector3f &vector) { return {vector.x(), vector.y(), vector.z()}; }

Eigen::Matrix4f make_scaling_mat4f(float scale_factor) {
  Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
  m(0, 0) = scale_factor;
  m(1, 1) = scale_factor;
  m(2, 2) = scale_factor;
  return m;
}

std::string to_string(const Eigen::Matrix4f &matrix) {
  std::stringstream ss;
  ss << matrix;
  return ss.str();
}

} // namespace MR::EigenHelpers
