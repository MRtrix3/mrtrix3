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

#include "dwi/tractography/formats/qfib_codec.h"
#include "dwi/tractography/streamline.h"
#include "exception.h"
#include "math/math.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

using MR::DWI::Tractography::Streamline;
namespace QFibCodec = MR::DWI::Tractography::Formats::QFibCodec;
using QFibCodec::BitDepth;

namespace {

//! \brief angle (radians) between two (not necessarily unit) vectors.
double angle_between(const Eigen::Vector3d &a, const Eigen::Vector3d &b) {
  const double c = a.normalized().dot(b.normalized());
  return std::acos(std::min(1.0, std::max(-1.0, c)));
}

//! \brief a sphere-covering set of unit directions for round-trip coverage.
std::vector<Eigen::Vector3d> sphere_directions() {
  std::vector<Eigen::Vector3d> dirs;
  for (int i = 0; i != 12; ++i) {
    const double phi = 2.0 * MR::Math::pi * i / 12.0;
    for (int j = 1; j != 12; ++j) {
      const double theta = MR::Math::pi * j / 12.0;
      dirs.emplace_back(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
    }
  }
  dirs.emplace_back(0, 0, 1);
  dirs.emplace_back(0, 0, -1);
  return dirs;
}

//! \brief a constant-stepsize helix sampled at a fixed parameter increment.
Streamline<float> make_helix(size_t n, double radius, double pitch, double dtheta) {
  Streamline<float> tck;
  for (size_t i = 0; i != n; ++i) {
    const double a = static_cast<double>(i) * dtheta;
    tck.push_back(Eigen::Vector3f(static_cast<float>(radius * std::cos(a)),
                                  static_cast<float>(radius * std::sin(a)),
                                  static_cast<float>(pitch * a)));
  }
  return tck;
}

} // namespace

/* ************************************************************************ */
/*                       Octahedral quantization                          */
/* ************************************************************************ */

TEST(QFibOctahedral, RoundTrip16Bit) {
  for (const Eigen::Vector3d &u : sphere_directions()) {
    const int32_t index = QFibCodec::octahedral_encode(u, BitDepth::M16);
    const Eigen::Vector3d decoded = QFibCodec::octahedral_decode(index, BitDepth::M16);
    EXPECT_NEAR(decoded.norm(), 1.0, 1e-9);
    // 8 bits per coordinate (256 levels): sub-degree angular resolution.
    EXPECT_LT(angle_between(u, decoded), 0.03);
  }
}

TEST(QFibOctahedral, RoundTrip8Bit) {
  for (const Eigen::Vector3d &u : sphere_directions()) {
    const int32_t index = QFibCodec::octahedral_encode(u, BitDepth::M8);
    const Eigen::Vector3d decoded = QFibCodec::octahedral_decode(index, BitDepth::M8);
    EXPECT_NEAR(decoded.norm(), 1.0, 1e-9);
    // 4 bits per coordinate (16 levels): coarse but bounded resolution.
    EXPECT_LT(angle_between(u, decoded), 0.5);
  }
}

TEST(QFibOctahedral, IndexFitsWidth) {
  for (const Eigen::Vector3d &u : sphere_directions()) {
    const int32_t i8 = QFibCodec::octahedral_encode(u, BitDepth::M8);
    EXPECT_GE(i8, 0);
    EXPECT_LE(i8, 255);
    const int32_t i16 = QFibCodec::octahedral_encode(u, BitDepth::M16);
    EXPECT_GE(i16, 0);
    EXPECT_LE(i16, 65535);
  }
}

/* ************************************************************************ */
/*                  Cap <-> sphere mapping (Rousseau-Boubekeur)           */
/* ************************************************************************ */

TEST(QFibMapping, InverseIsExact) {
  const double ratio = QFibCodec::ratio_from_angle(MR::Math::pi / 4.0); // psi = 45 degrees
  const Eigen::Vector3d axis = Eigen::Vector3d(1.0, 2.0, -0.5).normalized();
  // Two unit vectors spanning the plane perpendicular to the axis.
  const Eigen::Vector3d e1 = axis.unitOrthogonal();
  const Eigen::Vector3d e2 = axis.cross(e1);

  for (int t = 0; t != 8; ++t) {
    const double theta = (MR::Math::pi / 4.0) * t / 8.0; // strictly within the cap
    for (int p = 0; p != 8; ++p) {
      const double phi = 2.0 * MR::Math::pi * p / 8.0;
      const Eigen::Vector3d cap_dir =
          std::cos(theta) * axis + std::sin(theta) * (std::cos(phi) * e1 + std::sin(phi) * e2);
      const Eigen::Vector3d sphere = QFibCodec::inverse_mapping(cap_dir, axis, ratio);
      const Eigen::Vector3d back = QFibCodec::mapping(sphere, axis, ratio);
      EXPECT_NEAR(back.norm(), 1.0, 1e-9);
      EXPECT_LT(angle_between(cap_dir, back), 1e-6);
    }
  }
}

TEST(QFibMapping, RatioAngleRoundTrip) {
  for (double psi_deg : {10.0, 30.0, 45.0, 60.0, 90.0}) {
    const double psi = psi_deg * MR::Math::pi / 180.0;
    const double ratio = QFibCodec::ratio_from_angle(psi);
    EXPECT_NEAR(QFibCodec::angle_from_ratio(ratio), psi, 1e-9);
  }
}

/* ************************************************************************ */
/*                    Constant-stepsize detection                         */
/* ************************************************************************ */

TEST(QFibStepsize, UniformReturnsDelta) {
  const Streamline<float> tck = make_helix(20, 5.0, 0.5, 0.1);
  const std::optional<double> delta = QFibCodec::constant_stepsize<float>(tck, 1e-3);
  ASSERT_TRUE(delta.has_value());
  const double expected = (tck[1].cast<double>() - tck[0].cast<double>()).norm();
  EXPECT_NEAR(delta.value(), expected, 1e-5);
}

TEST(QFibStepsize, PerturbedReturnsNullopt) {
  Streamline<float> tck = make_helix(20, 5.0, 0.5, 0.1);
  // Displace one interior vertex so a segment length departs from the rest.
  tck[10] += Eigen::Vector3f(3.0F, 0.0F, 0.0F);
  EXPECT_FALSE(QFibCodec::constant_stepsize<float>(tck, 1e-3).has_value());
}

// A fractional terminal segment (as left by downsampling or end-cropping) is
//   measured against the interior mean and rejected, even though the interior is
//   perfectly uniform.
TEST(QFibStepsize, FractionalTerminalReturnsNullopt) {
  Streamline<float> tck = make_helix(20, 5.0, 0.5, 0.1);
  // Halve the last segment by moving the final vertex toward its predecessor.
  tck.back() = Eigen::Vector3f(0.5F * (tck[tck.size() - 2] + tck.back()));
  EXPECT_FALSE(QFibCodec::constant_stepsize<float>(tck, 5e-2).has_value());

  Streamline<float> tck2 = make_helix(20, 5.0, 0.5, 0.1);
  // Halve the first segment likewise.
  tck2[0] = Eigen::Vector3f(0.5F * (tck2[0] + tck2[1]));
  EXPECT_FALSE(QFibCodec::constant_stepsize<float>(tck2, 5e-2).has_value());
}

/* ************************************************************************ */
/*                    Per-streamline compress/decompress                  */
/* ************************************************************************ */

TEST(QFibCompress, RoundTripWithinTolerance) {
  const Streamline<float> tck = make_helix(40, 5.0, 0.5, 0.08);
  const double ratio = QFibCodec::ratio_from_angle(MR::Math::pi / 2.0);
  const QFibCodec::Compressed<float> c = QFibCodec::compress<float>(tck, BitDepth::M16, ratio, 1e-3);
  ASSERT_EQ(c.indices.size(), tck.size() - 2);

  const Streamline<float> back = QFibCodec::decompress<float>(c, BitDepth::M16, ratio);
  ASSERT_EQ(back.size(), tck.size());
  // The two seed points are stored verbatim.
  EXPECT_LT((back[0] - tck[0]).norm(), 1e-5);
  EXPECT_LT((back[1] - tck[1]).norm(), 1e-5);
  // Quantization error stays bounded along the streamline.
  double max_error = 0.0;
  for (size_t i = 0; i != tck.size(); ++i)
    max_error = std::max(max_error, static_cast<double>((back[i] - tck[i]).norm()));
  EXPECT_LT(max_error, 0.1);
}

TEST(QFibCompress, ReEncodingIsIdempotent) {
  const Streamline<float> tck = make_helix(40, 5.0, 0.5, 0.08);
  const double ratio = QFibCodec::ratio_from_angle(MR::Math::pi / 2.0);
  const QFibCodec::Compressed<float> c1 = QFibCodec::compress<float>(tck, BitDepth::M16, ratio, 1e-3);
  const Streamline<float> back = QFibCodec::decompress<float>(c1, BitDepth::M16, ratio);
  // Re-compressing the decompressed streamline reproduces the same indices: the
  //   encoder measures each direction from the already-decoded previous point.
  const QFibCodec::Compressed<float> c2 = QFibCodec::compress<float>(back, BitDepth::M16, ratio, 1e-3);
  ASSERT_EQ(c1.indices.size(), c2.indices.size());
  EXPECT_EQ(c1.indices, c2.indices);
}

TEST(QFibCompress, RejectsNonConstantStepsize) {
  Streamline<float> tck = make_helix(20, 5.0, 0.5, 0.1);
  tck[10] += Eigen::Vector3f(3.0F, 0.0F, 0.0F);
  const double ratio = QFibCodec::ratio_from_angle(MR::Math::pi / 2.0);
  EXPECT_THROW(QFibCodec::compress<float>(tck, BitDepth::M16, ratio, 1e-3), MR::Exception);
}

TEST(QFibCompress, RejectsTooShort) {
  Streamline<float> tck;
  tck.push_back(Eigen::Vector3f(0, 0, 0));
  const double ratio = QFibCodec::ratio_from_angle(MR::Math::pi / 2.0);
  EXPECT_THROW(QFibCodec::compress<float>(tck, BitDepth::M16, ratio, 1e-3), MR::Exception);
}
