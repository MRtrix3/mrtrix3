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

#include <algorithm>
#include <cmath>

#include "exception.h"
#include "mrtrix.h"

namespace MR::DWI::Tractography::Formats::QFibCodec {

namespace {

//! \brief the small bias added to the cap-area ratio (keeps the cap clear of the antipode).
constexpr double ratio_epsilon = 1.0e-3;
//! \brief below this perpendicular-component norm a direction is treated as along the pole.
constexpr double pole_tolerance = 1.0e-12;

//! \brief number of bits spent on each of the two octahedral coordinates.
constexpr int half_bits(BitDepth bits) noexcept { return static_cast<int>(bits) / 2; }

//! \brief sign of \a v, with zero mapped to +1 (octahedral unwrap convention).
constexpr double sign_not_zero(double v) noexcept { return v >= 0.0 ? 1.0 : -1.0; }

} // namespace

/* ************************************************************************ */
/*                  Octahedral unit-vector quantization                   */
/* ************************************************************************ */

int32_t octahedral_encode(const Eigen::Vector3d &unit, BitDepth bits) noexcept {
  Eigen::Vector3d n = unit;
  const double l1 = std::fabs(n.x()) + std::fabs(n.y()) + std::fabs(n.z());
  if (l1 > 0.0)
    n /= l1;

  double u = n.x();
  double v = n.y();
  if (n.z() < 0.0) {
    const double folded_u = (1.0 - std::fabs(v)) * sign_not_zero(u);
    const double folded_v = (1.0 - std::fabs(u)) * sign_not_zero(v);
    u = folded_u;
    v = folded_v;
  }

  // Snap each coordinate in [-1, 1] to an unsigned integer of half_bits width,
  //   then pack the pair into one index (low coordinate in the least-significant
  //   half) that fits an int8 (M8) or int16 (M16) on disk.
  const int32_t span = (1 << half_bits(bits)) - 1;
  const auto quantize = [span](double coordinate) noexcept -> int32_t {
    const double clamped = std::min(1.0, std::max(-1.0, coordinate));
    return static_cast<int32_t>(std::lround((clamped + 1.0) * 0.5 * span));
  };
  const int32_t qu = quantize(u);
  const int32_t qv = quantize(v);
  return qu * (span + 1) + qv;
}

Eigen::Vector3d octahedral_decode(int32_t index, BitDepth bits) noexcept {
  const int32_t base = 1 << half_bits(bits);
  const int32_t span = base - 1;
  const int32_t qu = (index / base) % base;
  const int32_t qv = index % base;

  const auto dequantize = [span](int32_t q) noexcept -> double {
    return (static_cast<double>(q) / static_cast<double>(span)) * 2.0 - 1.0;
  };
  double u = dequantize(qu);
  double v = dequantize(qv);

  Eigen::Vector3d n(u, v, 1.0 - std::fabs(u) - std::fabs(v));
  if (n.z() < 0.0) {
    const double folded_x = (1.0 - std::fabs(n.y())) * sign_not_zero(n.x());
    const double folded_y = (1.0 - std::fabs(n.x())) * sign_not_zero(n.y());
    n.x() = folded_x;
    n.y() = folded_y;
  }
  n.normalize();
  return n;
}

/* ************************************************************************ */
/*               Rousseau-Boubekeur cap <-> sphere mapping                 */
/* ************************************************************************ */

namespace {

//! \brief remap a direction along \a axis given a new cosine of the polar angle.
/*! Shared body of mapping()/inverse_mapping(): decompose \a dir into its
 * component along \a axis and its perpendicular (azimuthal) direction, then
 * rebuild a unit vector at polar angle acos(\a new_cos) about \a axis preserving
 * that azimuth. */
Eigen::Vector3d remap_polar(const Eigen::Vector3d &dir, const Eigen::Vector3d &axis, double new_cos) noexcept {
  const double clamped_cos = std::min(1.0, std::max(-1.0, new_cos));
  const Eigen::Vector3d perpendicular = dir - dir.dot(axis) * axis;
  const double perpendicular_norm = perpendicular.norm();
  if (perpendicular_norm < pole_tolerance)
    return clamped_cos >= 0.0 ? axis : Eigen::Vector3d(-axis);
  const Eigen::Vector3d unit_perpendicular = perpendicular / perpendicular_norm;
  const double sin_angle = std::sqrt(std::max(0.0, 1.0 - clamped_cos * clamped_cos));
  return clamped_cos * axis + sin_angle * unit_perpendicular;
}

} // namespace

Eigen::Vector3d inverse_mapping(const Eigen::Vector3d &cap_dir, const Eigen::Vector3d &axis, double ratio) noexcept {
  const double cos_cap = std::min(1.0, std::max(-1.0, cap_dir.dot(axis)));
  const double cos_sphere = 1.0 - (1.0 - cos_cap) / ratio;
  return remap_polar(cap_dir, axis, cos_sphere);
}

Eigen::Vector3d mapping(const Eigen::Vector3d &sphere_dir, const Eigen::Vector3d &axis, double ratio) noexcept {
  const double cos_sphere = std::min(1.0, std::max(-1.0, sphere_dir.dot(axis)));
  const double cos_cap = 1.0 - ratio * (1.0 - cos_sphere);
  return remap_polar(sphere_dir, axis, cos_cap);
}

double ratio_from_angle(double psi_radians) noexcept { return 0.5 * (1.0 - std::cos(psi_radians)) + ratio_epsilon; }

double angle_from_ratio(double ratio) noexcept {
  const double cos_psi = std::min(1.0, std::max(-1.0, 1.0 - 2.0 * (ratio - ratio_epsilon)));
  return std::acos(cos_psi);
}

/* ************************************************************************ */
/*                    Per-streamline compress/decompress                  */
/* ************************************************************************ */

template <class V> std::optional<double> constant_stepsize(const Streamline<V> &tck, double tolerance) {
  if (tck.size() < 2)
    return std::nullopt;

  const size_t num_segments = tck.size() - 1;
  const auto segment_length = [&tck](size_t j) -> double {
    return (tck[j + 1].template cast<double>() - tck[j].template cast<double>()).norm();
  };

  // The reference step size is the mean of the *interior* segments where any
  //   exist (i.e. excluding the first and last segments). The terminal segments
  //   of a tractogram routinely fall short of the tracking step (the seed and
  //   the termination point land partway through a step), so estimating the step
  //   from them — or from the first segment alone, as the on-disk model would —
  //   is unreliable; the interior mean is robust to that. Streamlines too short
  //   to have an interior segment fall back to the mean of all segments.
  double sum = 0.0;
  size_t count = 0;
  if (num_segments > 2) {
    for (size_t j = 1; j + 1 != num_segments; ++j) {
      sum += segment_length(j);
      ++count;
    }
  } else {
    for (size_t j = 0; j != num_segments; ++j) {
      sum += segment_length(j);
      ++count;
    }
  }
  const double delta = sum / static_cast<double>(count);
  if (delta <= 0.0)
    return std::nullopt;

  // Every segment, terminals included, must match the reference step: a fractional
  //   terminal (from downsampling or end-cropping) or a varying interior step
  //   makes the streamline unrepresentable by the fixed-step model.
  for (size_t j = 0; j != num_segments; ++j) {
    if (std::fabs(segment_length(j) - delta) > tolerance * delta)
      return std::nullopt;
  }
  return delta;
}

template <class V> Compressed<V> compress(const Streamline<V> &tck, BitDepth bits, double ratio, double step_tol) {
  if (tck.size() < 2)
    throw Exception("cannot QFib-compress a streamline of fewer than two vertices");

  if (!constant_stepsize(tck, step_tol).has_value())
    throw Exception("streamline does not have a constant step size,"
                    " which the QFib format is not able to represent");

  Compressed<V> out;
  out.first = tck[0];
  out.second = tck[1];
  out.indices.reserve(tck.size() - 2);

  // The on-disk model derives the step size from the first two stored vertices,
  //   so the encoder must advance along that same step for the decoder to track it.
  const double stepsize = (tck[1].template cast<double>() - tck[0].template cast<double>()).norm();
  Eigen::Vector3d axis = (tck[1].template cast<double>() - tck[0].template cast<double>()).normalized();
  Eigen::Vector3d current = tck[1].template cast<double>();

  // Each direction is measured from the already-decoded position \c current and
  //   quantized in the cap about the previous decoded tangent \c axis; the axis
  //   and the running position then advance along the *decoded* tangent so the
  //   encoder tracks exactly what the decoder will reconstruct.
  for (size_t i = 2; i != tck.size(); ++i) {
    const Eigen::Vector3d tangent = (tck[i].template cast<double>() - current).normalized();
    const int32_t index = octahedral_encode(inverse_mapping(tangent, axis, ratio), bits);
    out.indices.push_back(index);
    axis = mapping(octahedral_decode(index, bits), axis, ratio);
    current += stepsize * axis;
  }
  return out;
}

template <class V> Streamline<V> decompress(const Compressed<V> &cfiber, BitDepth bits, double ratio) {
  Streamline<V> tck;
  tck.reserve(cfiber.indices.size() + 2);
  tck.push_back(cfiber.first);
  tck.push_back(cfiber.second);

  const Eigen::Vector3d origin = cfiber.first.template cast<double>();
  const Eigen::Vector3d second = cfiber.second.template cast<double>();
  const double stepsize = (second - origin).norm();
  Eigen::Vector3d axis = (second - origin).normalized();

  for (const int32_t index : cfiber.indices) {
    const Eigen::Vector3d tangent = mapping(octahedral_decode(index, bits), axis, ratio);
    const Eigen::Vector3d next = tck.back().template cast<double>() + stepsize * tangent;
    tck.push_back(next.template cast<V>());
    axis = tangent;
  }
  return tck;
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template std::optional<double> constant_stepsize<float>(const Streamline<float> &, double);
template std::optional<double> constant_stepsize<double>(const Streamline<double> &, double);
template Compressed<float> compress<float>(const Streamline<float> &, BitDepth, double, double);
template Compressed<double> compress<double>(const Streamline<double> &, BitDepth, double, double);
template Streamline<float> decompress<float>(const Compressed<float> &, BitDepth, double);
template Streamline<double> decompress<double>(const Compressed<double> &, BitDepth, double);

} // namespace MR::DWI::Tractography::Formats::QFibCodec
