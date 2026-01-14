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

#include "gpu/registration/registrationtypes.h"
#include <tcb/span.hpp>
#include "types.h"

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/QR>

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace MR {
// using transform_type = Eigen::Transform<default_type, 3, Eigen::AffineCompact>;
namespace {
constexpr size_t param_count_for_type(TransformationType type) { return type == TransformationType::Rigid ? 6U : 12U; }
} // namespace

TransformationType GlobalTransform::type() const { return m_type; }

tcb::span<const float> GlobalTransform::parameters() const {
  return tcb::span<const float>(m_params.data(), m_param_count);
}

void GlobalTransform::set_params(tcb::span<const float> params) {
  const size_t expected = param_count_for_type(m_type);
  if (params.size() != expected) {
    throw std::invalid_argument("Parameter count does not match transformation type.");
  }
  std::copy(params.begin(), params.end(), m_params.begin());
  m_param_count = expected;
}

Eigen::Vector3f GlobalTransform::pivot() const { return m_pivot; }

void GlobalTransform::set_pivot(const Eigen::Vector3f &pivot) { m_pivot = pivot; }

transform_type GlobalTransform::to_affine_compact() const {
  const Eigen::Translation3f translateToPivot(-m_pivot.x(), -m_pivot.y(), -m_pivot.z());
  const Eigen::Translation3f translateFromPivot(m_pivot.x(), m_pivot.y(), m_pivot.z());

  Eigen::Transform<float, 3, Eigen::Affine> shear = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
  if (is_affine()) {
    // The linear part of the shear matrix:
    // [ 1  sh_xy sh_xz ]
    // [ 0  1    sh_yz ]
    // [ 0  0    1     ]
    shear.matrix()(0, 1) = m_params[9];
    shear.matrix()(0, 2) = m_params[10];
    shear.matrix()(1, 2) = m_params[11];
  }

  Eigen::Transform<float, 3, Eigen::Affine> scale = Eigen::Transform<float, 3, Eigen::Affine>::Identity();
  if (is_affine()) {
    scale = Eigen::Scaling(m_params[6], m_params[7], m_params[8]);
  }

  const Eigen::Vector3f rotationAxisAngleVec(m_params[3], m_params[4], m_params[5]);
  const float angle = rotationAxisAngleVec.norm();
  Eigen::AngleAxisf rotation = Eigen::AngleAxisf::Identity();
  if (angle != 0.0f) {
    rotation = Eigen::AngleAxisf(angle, rotationAxisAngleVec / angle);
  }

  const Eigen::Translation3f globalTranslation(m_params[0], m_params[1], m_params[2]);

  // Combine transformations in the correct order:
  // M_final = M6 * M5 * M4 * M3 * M2 * M1
  // (Applied to a point P as M_final * P)
  const Eigen::Transform<float, 3, Eigen::AffineCompact> final_affine_transform =
      globalTranslation * translateFromPivot * rotation * scale * shear * translateToPivot;

  return transform_type(final_affine_transform);
}

GlobalTransform::GlobalTransform(tcb::span<const float> params, TransformationType type, const Eigen::Vector3f &pivot)
    : m_type(type), m_pivot(pivot) {
  const size_t expected = param_count_for_type(type);
  if (params.size() != expected) {
    throw std::invalid_argument("Parameter count does not match transformation type.");
  }
  m_params.fill(0.0F);
  std::copy(params.begin(), params.end(), m_params.begin());
  m_param_count = expected;
}

Eigen::Matrix4f GlobalTransform::to_matrix4f() const {
  transform_type transform = to_affine_compact();
  Eigen::Matrix4f matrix = Eigen::Matrix4f::Identity();
  matrix.block<3, 4>(0, 0) = transform.matrix().template cast<float>();
  return matrix;
}

GlobalTransform GlobalTransform::inverse() const {
  const auto eigenTransform = to_affine_compact();
  const auto inverseEigenTransform = eigenTransform.inverse();
  return GlobalTransform::from_affine_compact(inverseEigenTransform, m_pivot, m_type);
}

GlobalTransform GlobalTransform::with_pivot(const Eigen::Vector3f &pivot) const {
  return GlobalTransform(parameters(), m_type, pivot);
}
// Keeps translation and axis-angle rotation while dropping scale/shear.
GlobalTransform GlobalTransform::as_rigid() const {
  if (is_rigid()) {
    return *this;
  }
  const std::array<float, 6> rigid_params{m_params[0], m_params[1], m_params[2], m_params[3], m_params[4], m_params[5]};
  return GlobalTransform(rigid_params, TransformationType::Rigid, m_pivot);
}

// Ensures scale defaults to 1 and shear to 0 when promoting a rigid transform to affine.
GlobalTransform GlobalTransform::as_affine() const {
  if (is_affine()) {
    return *this;
  }
  std::array<float, 12> affine_params{};
  const auto current = parameters();
  std::copy(current.begin(), current.end(), affine_params.begin());
  affine_params[6] = 1.0F;
  affine_params[7] = 1.0F;
  affine_params[8] = 1.0F;
  return GlobalTransform(affine_params, TransformationType::Affine, m_pivot);
}

bool GlobalTransform::is_rigid() const { return m_type == TransformationType::Rigid; }

bool GlobalTransform::is_affine() const { return m_type == TransformationType::Affine; }

size_t GlobalTransform::param_count() const { return m_param_count; }

void GlobalTransform::set_translation(const Eigen::Vector3f &translation) {
  m_params[0] = translation.x();
  m_params[1] = translation.y();
  m_params[2] = translation.z();
}

Eigen::Vector3f GlobalTransform::translation() const { return Eigen::Vector3f(m_params[0], m_params[1], m_params[2]); }

void GlobalTransform::set_rotation(const Eigen::Vector3f &rotation_axis_angle) {
  m_params[3] = rotation_axis_angle.x();
  m_params[4] = rotation_axis_angle.y();
  m_params[5] = rotation_axis_angle.z();
}

Eigen::Vector3f GlobalTransform::rotation() const { return Eigen::Vector3f(m_params[3], m_params[4], m_params[5]); }

void GlobalTransform::set_scale(const Eigen::Vector3f &scale) {
  if (is_rigid()) {
    throw std::logic_error("Scale is only valid for affine transforms.");
  }
  m_params[6] = scale.x();
  m_params[7] = scale.y();
  m_params[8] = scale.z();
}

Eigen::Vector3f GlobalTransform::scale() const {
  if (is_rigid()) {
    return Eigen::Vector3f(1.0F, 1.0F, 1.0F);
  }
  return Eigen::Vector3f(m_params[6], m_params[7], m_params[8]);
}

void GlobalTransform::set_shear(const Eigen::Vector3f &shear) {
  if (is_rigid()) {
    throw std::logic_error("Shear is only valid for affine transforms.");
  }
  m_params[9] = shear.x();
  m_params[10] = shear.y();
  m_params[11] = shear.z();
}

Eigen::Vector3f GlobalTransform::shear() const {
  if (is_rigid()) {
    return Eigen::Vector3f::Zero();
  }
  return Eigen::Vector3f(m_params[9], m_params[10], m_params[11]);
}

// Decomposes a 4x4 affine transformation matrix back into its constituent parameters
// (translation, rotation, scale, shear) defined relative to a pivot point.
//
// The forward transformation is composed as: p' = T_global * T_to_pivot * R * S * Sh * T_from_pivot * p
// This can be expressed as a standard affine matrix: p' = (LinearPart * p) + TranslationPart

// The full translation vector is derived from: T_full = T_global - LinearPart*pivot + pivot
// Rearranging gives: T_global = T_full - pivot + LinearPart*pivot
//
// Linear Part Decomposition (to find R, S, Sh):
// The linear part is a product: LinearPart = R * (S * Sh). The (Scale * Shear) term
// forms an upper-triangular matrix U. This means LinearPart = R * U.
// A QR decomposition splits a matrix into an
// orthogonal matrix Q (our rotation R) and an upper-triangular matrix R_qr (U).
// For Affine (N=12): We perform the QR decomposition. Scale values are on the diagonal
// of R_qr, and shear values are the normalized off-diagonals.
// For Rigid (N=6): S and Sh are identity matrices, so the LinearPart is already the
// pure rotation matrix R. No decomposition is needed.

// TODO: we need to write unit tests for this function.
// TODO: also should we use Eigen::ColPivHouseholderQR instead?
GlobalTransform GlobalTransform::from_affine_compact(const transform_type &transform,
                                                     const Eigen::Vector3f &pivot,
                                                     TransformationType type) {
  using Scalar = typename transform_type::Scalar;
  using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
  using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;

  const Vector3 pivotVector = pivot.template cast<Scalar>();

  const Matrix3 linearPart = transform.linear();
  const Vector3 translationPart = transform.translation();
  const Vector3 globalTranslation = translationPart - pivotVector + linearPart * pivotVector;

  std::vector<float> parameters;
  const size_t N = (type == TransformationType::Rigid) ? 6 : 12;
  parameters.resize(N, 0.0F);

  parameters[0] = static_cast<float>(globalTranslation.x());
  parameters[1] = static_cast<float>(globalTranslation.y());
  parameters[2] = static_cast<float>(globalTranslation.z());

  Matrix3 rotationMatrix;

  if (N == 12) {
    // Decompose the linear part using QR decomposition
    const Eigen::HouseholderQR<Matrix3> qr(linearPart);
    rotationMatrix = qr.householderQ();
    Matrix3 upperTriangularPart = qr.matrixQR().template triangularView<Eigen::Upper>();

    // Ensure the result is a proper rotation matrix (determinant = +1), not a reflection.
    if (rotationMatrix.determinant() < Scalar(0)) {
      rotationMatrix.col(0) *= -1;
      upperTriangularPart.row(0) *= -1;
    }

    // Force positive diagonal on R as we don't want negative scales.
    for (int i = 0; i < 3; ++i) {
      if (upperTriangularPart(i, i) < Scalar(0)) {
        rotationMatrix.col(i) *= -1;
        upperTriangularPart.row(i) *= -1;
      }
    }

    // Ensure Q is proper after diagonal fix
    if (rotationMatrix.determinant() < Scalar(0)) {
      rotationMatrix.col(2) *= -1;      // flip one column (e.g. the last)
      upperTriangularPart.row(2) *= -1; // and the matching row in R
    }

    // Extract scale and shear from the upper triangular matrix
    const Scalar scaleX = upperTriangularPart(0, 0);
    const Scalar scaleY = upperTriangularPart(1, 1);
    const Scalar scaleZ = upperTriangularPart(2, 2);
    parameters[6] = static_cast<float>(scaleX);
    parameters[7] = static_cast<float>(scaleY);
    parameters[8] = static_cast<float>(scaleZ);

    parameters[9] = (scaleX != 0) ? static_cast<float>(upperTriangularPart(0, 1) / scaleX) : 0.0F;
    parameters[10] = (scaleX != 0) ? static_cast<float>(upperTriangularPart(0, 2) / scaleX) : 0.0F;
    parameters[11] = (scaleY != 0) ? static_cast<float>(upperTriangularPart(1, 2) / scaleY) : 0.0F;

  } else {
    // For the rigid case the linear part is the rotation matrix. No decomposition is needed.
    rotationMatrix = linearPart;
  }

  Eigen::AngleAxis<Scalar> angleAxis(rotationMatrix);
  const Vector3 axisAngleVector = angleAxis.axis() * angleAxis.angle();
  parameters[3] = static_cast<float>(axisAngleVector.x());
  parameters[4] = static_cast<float>(axisAngleVector.y());
  parameters[5] = static_cast<float>(axisAngleVector.z());

  return GlobalTransform(parameters, type, pivot);
}
} // namespace MR
