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

#include "registration/warp/padded_field.h"

#include <Eigen/Core>
#include <gtest/gtest.h>

#include "header.h"
#include "image.h"
#include "interp/warp.h"

#include "algo/loop.h"

using namespace MR;
using namespace MR::Registration::Warp;

namespace {

//! a 4D warp-field header (3 volumes along axis 3) on a unit-spacing identity grid
Header make_warp_header(const ssize_t nx, const ssize_t ny, const ssize_t nz) {
  Header header;
  header.ndim() = 4;
  header.size(0) = nx;
  header.size(1) = ny;
  header.size(2) = nz;
  header.size(3) = 3;
  for (ssize_t axis = 0; axis != 4; ++axis)
    header.spacing(axis) = 1.0;
  header.stride(0) = 1;
  header.stride(1) = 2;
  header.stride(2) = 3;
  header.stride(3) = 4;
  header.transform().setIdentity();
  header.datatype() = DataType::native(DataType::Float64);
  return header;
}

//! fill a warp field with the affine map value(voxel) = b + A * voxel_index
void fill_affine(Image<default_type> &field, const Eigen::Vector3d &b, const Eigen::Matrix3d &A) {
  for (auto l = Loop(field, 0, 3)(field); l; ++l) {
    const Eigen::Vector3d o{static_cast<default_type>(field.index(0)),
                            static_cast<default_type>(field.index(1)),
                            static_cast<default_type>(field.index(2))};
    const Eigen::Vector3d value = b + A * o;
    for (ssize_t component = 0; component != 3; ++component) {
      field.index(3) = component;
      field.value() = value[component];
    }
  }
}

} // namespace

// Linear extrapolation reproduces an affine field exactly across the whole padded buffer:
//   the interior is a verbatim copy, and the border shell carries the (constant) boundary
//   gradient, so every padded voxel equals the affine value at its original-grid coordinate.
TEST(PaddedField, AffineExactAcrossHalo) {
  Header header = make_warp_header(6, 5, 7);
  Image<default_type> field = Image<default_type>::scratch(header, "affine warp field");
  const Eigen::Vector3d b(1.5, -2.0, 3.25);
  Eigen::Matrix3d A;
  A << 0.10, -0.20, 0.05, //
      0.30, 0.15, -0.10,  //
      -0.05, 0.25, 0.20;
  fill_affine(field, b, A);

  PaddedField padded(header);
  padded.refresh(field);

  Image<default_type> buffer = padded.buffer();
  const std::array<ssize_t, 3> &shift = padded.shift();
  for (auto l = Loop(buffer, 0, 3)(buffer); l; ++l) {
    const Eigen::Vector3d o{static_cast<default_type>(buffer.index(0) - shift[0]),
                            static_cast<default_type>(buffer.index(1) - shift[1]),
                            static_cast<default_type>(buffer.index(2) - shift[2])};
    const Eigen::Vector3d expected = b + A * o;
    for (ssize_t component = 0; component != 3; ++component) {
      buffer.index(3) = component;
      EXPECT_NEAR(buffer.value(), expected[component], 1e-9);
    }
  }
}

// DenseWarp cubic-interpolates the padded buffer. Cubic interpolation reproduces an affine
//   field exactly, and because the border shell is exact too, even sub-voxel positions near
//   the field-of-view edge recover the affine value.
TEST(DenseWarp, AffineExactInterpolation) {
  Header header = make_warp_header(6, 5, 7);
  Image<default_type> field = Image<default_type>::scratch(header, "affine warp field");
  const Eigen::Vector3d b(0.5, 4.0, -1.0);
  Eigen::Matrix3d A;
  A << 0.20, 0.10, -0.05, //
      -0.15, 0.25, 0.10,  //
      0.05, -0.20, 0.30;
  fill_affine(field, b, A);

  PaddedField padded(header);
  padded.refresh(field);
  Interp::DenseWarp<default_type> warp(padded.buffer(), padded.shift(), padded.original_size());

  const std::array<ssize_t, 3> &shift = padded.shift();
  // Sample at sub-voxel positions spanning the interior, including one within half a voxel of
  //   the field-of-view edge (whose cubic kernel reads into the extrapolated shell).
  for (const Eigen::Vector3d &original_voxel :
       {Eigen::Vector3d(2.3, 1.7, 3.1), Eigen::Vector3d(0.0, 0.0, 0.0), Eigen::Vector3d(4.9, 3.9, 5.9)}) {
    const Eigen::Vector3d padded_voxel = original_voxel + Eigen::Vector3d(static_cast<default_type>(shift[0]),
                                                                          static_cast<default_type>(shift[1]),
                                                                          static_cast<default_type>(shift[2]));
    ASSERT_TRUE(warp.voxel(padded_voxel));
    const Eigen::Vector3d got = warp.row(3);
    const Eigen::Vector3d expected = b + A * original_voxel;
    for (ssize_t component = 0; component != 3; ++component)
      EXPECT_NEAR(got[component], expected[component], 1e-7);
  }
}

// Acceptance is the original (pre-padding) field of view: positions beyond it are rejected
//   even though the padded buffer holds (extrapolated) data there.
TEST(DenseWarp, AcceptanceIsOriginalFieldOfView) {
  Header header = make_warp_header(6, 5, 7);
  Image<default_type> field = Image<default_type>::scratch(header, "affine warp field");
  fill_affine(field, Eigen::Vector3d::Zero(), Eigen::Matrix3d::Identity());

  PaddedField padded(header);
  padded.refresh(field);
  Interp::DenseWarp<default_type> warp(padded.buffer(), padded.shift(), padded.original_size());

  const Eigen::Vector3d s(static_cast<default_type>(padded.shift()[0]),
                          static_cast<default_type>(padded.shift()[1]),
                          static_cast<default_type>(padded.shift()[2]));

  // Just inside the lower and upper field-of-view edges: accepted.
  EXPECT_TRUE(warp.voxel(s + Eigen::Vector3d(-0.49, 0.0, 0.0)));
  EXPECT_TRUE(warp.voxel(s + Eigen::Vector3d(5.49, 4.49, 6.49)));
  // Just outside the lower and upper edges (still within the populated halo): rejected.
  EXPECT_FALSE(warp.voxel(s + Eigen::Vector3d(-0.51, 0.0, 0.0)));
  EXPECT_FALSE(warp.voxel(s + Eigen::Vector3d(5.51, 0.0, 0.0)));
}
