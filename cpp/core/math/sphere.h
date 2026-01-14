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

#pragma once

#include <cmath>
#include <sys/types.h>
#include <type_traits>

#include <Eigen/Core>

#include "math/math.h"

namespace MR::Math::Sphere {

//! convert spherical coordinates to Cartesian coordinates
template <class VectorType1, class VectorType2>
inline typename std::enable_if<VectorType1::IsVectorAtCompileTime, void>::type
spherical2cartesian(const VectorType1 &az_in_r, VectorType2 &&xyz) {
  if (az_in_r.size() == 3) {
    xyz[0] = az_in_r[2] * std::sin(az_in_r[1]) * std::cos(az_in_r[0]);
    xyz[1] = az_in_r[2] * std::sin(az_in_r[1]) * std::sin(az_in_r[0]);
    xyz[2] = az_in_r[2] * cos(az_in_r[1]);
  } else {
    xyz[0] = std::sin(az_in_r[1]) * std::cos(az_in_r[0]);
    xyz[1] = std::sin(az_in_r[1]) * std::sin(az_in_r[0]);
    xyz[2] = cos(az_in_r[1]);
  }
}

//! convert matrix of spherical coordinates to Cartesian coordinates
template <class MatrixType1, class MatrixType2>
inline typename std::enable_if<!MatrixType1::IsVectorAtCompileTime, void>::type
spherical2cartesian(const MatrixType1 &az_in, MatrixType2 &&cartesian) {
  cartesian.resize(az_in.rows(), 3);
  for (ssize_t dir = 0; dir < az_in.rows(); ++dir)
    spherical2cartesian(az_in.row(dir), cartesian.row(dir));
}

//! convert matrix of spherical coordinates to Cartesian coordinates
template <class MatrixType>
inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, Eigen::MatrixXd>::type
spherical2cartesian(const MatrixType &az_in) {
  Eigen::MatrixXd cartesian(az_in.rows(), 3);
  for (ssize_t dir = 0; dir < az_in.rows(); ++dir)
    spherical2cartesian(az_in.row(dir), cartesian.row(dir));
  return cartesian;
}

//! convert Cartesian coordinates to spherical coordinates
template <class VectorType1, class VectorType2>
inline typename std::enable_if<VectorType1::IsVectorAtCompileTime, void>::type
cartesian2spherical(const VectorType1 &xyz, VectorType2 &&az_in_r) {
  auto r = std::sqrt(Math::pow2(xyz[0]) + Math::pow2(xyz[1]) + Math::pow2(xyz[2]));
  az_in_r[0] = std::atan2(xyz[1], xyz[0]);
  az_in_r[1] = std::acos(xyz[2] / r);
  if (az_in_r.size() == 3)
    az_in_r[2] = r;
}

//! convert matrix of Cartesian coordinates to spherical coordinates
template <class MatrixType1, class MatrixType2>
inline typename std::enable_if<!MatrixType1::IsVectorAtCompileTime, void>::type
cartesian2spherical(const MatrixType1 &cartesian, MatrixType2 &&az_in, bool include_r = false) {
  az_in.allocate(cartesian.rows(), include_r ? 3 : 2);
  for (ssize_t dir = 0; dir < cartesian.rows(); ++dir)
    cartesian2spherical(cartesian.row(dir), az_in.row(dir));
}

//! convert matrix of Cartesian coordinates to spherical coordinates
template <class MatrixType>
inline typename std::enable_if<!MatrixType::IsVectorAtCompileTime, Eigen::MatrixXd>::type
cartesian2spherical(const MatrixType &cartesian, bool include_r = false) {
  Eigen::MatrixXd az_in(cartesian.rows(), include_r ? 3 : 2);
  for (ssize_t dir = 0; dir < cartesian.rows(); ++dir)
    cartesian2spherical(cartesian.row(dir), az_in.row(dir));
  return az_in;
}

//! normalise a set of Cartesian coordinates
template <class MatrixType> inline void normalise_cartesian(MatrixType &cartesian) {
  assert(cartesian.cols() == 3);
  for (ssize_t i = 0; i < cartesian.rows(); i++) {
    auto norm = cartesian.row(i).norm();
    if (norm)
      cartesian.row(i).array() /= norm;
  }
}

} // namespace MR::Math::Sphere
