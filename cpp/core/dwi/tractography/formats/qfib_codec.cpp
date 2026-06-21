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
#include "math/math.h"
#include "mrtrix.h"

namespace MR::DWI::Tractography::Formats::QFibCodec {

namespace {

//! \brief the small bias added to the cap-area ratio (keeps the cap clear of the antipode).
constexpr double ratio_epsilon = 1.0e-3;
//! \brief below this perpendicular-component norm a direction is treated as along the pole.
constexpr double pole_tolerance = 1.0e-12;

//! \brief number of bits spent on each of the two octahedral coordinates.
constexpr int half_bits(BitDepth bits) noexcept { return static_cast<int>(bits) / 2; }

//! \brief total number of distinct directions a Fibonacci word can encode (2^bits).
constexpr double fibonacci_lattice_size(BitDepth bits) noexcept {
  return static_cast<double>(uint32_t(1) << static_cast<int>(bits));
}

//! \brief the golden ratio, the generator of the spherical-Fibonacci spiral.
constexpr double golden_ratio = 1.6180339887498948482045868343656381;

//! \brief sign of \a v, with zero mapped to +1 (octahedral unwrap convention).
constexpr double sign_not_zero(double v) noexcept { return v >= 0.0 ? 1.0 : -1.0; }

//! \brief the Meyer-2010 lower-hemisphere unwrap of an octahedral (u, v) pair.
/*! Shared by the encode and decode of both octahedral schemes: when the implied z
 * is negative the pair is reflected across the octahedron diagonal. The operation
 * is its own structural inverse and identical in v1 and v2. */
Eigen::Vector2d octahedral_fold(double u, double v) noexcept {
  return {(1.0 - std::fabs(v)) * sign_not_zero(u), (1.0 - std::fabs(u)) * sign_not_zero(v)};
}

//! \brief the fractional part of \a x in [0, 1).
double fract(double x) noexcept { return x - std::floor(x); }

} // namespace

/* ************************************************************************ */
/*                  Octahedral unit-vector quantization                   */
/*                          (Meyer et al. 2010)                           */
/* ************************************************************************ */

int32_t octahedral_encode(const Eigen::Vector3d &unit, BitDepth bits) noexcept {
  Eigen::Vector3d n = unit;
  const double l1 = std::fabs(n.x()) + std::fabs(n.y()) + std::fabs(n.z());
  if (l1 > 0.0)
    n /= l1;

  double u = n.x();
  double v = n.y();
  if (n.z() < 0.0) {
    const Eigen::Vector2d folded = octahedral_fold(u, v);
    u = folded.x();
    v = folded.y();
  }

  // Quantize each coordinate to a *signed* integer of half_bits width: map [-1, 1]
  //   onto [-(2^(half-1)-1), +(2^(half-1)-1)] and store the pair as two's-complement
  //   sub-fields, the first in the high half of the word.
  const int half = half_bits(bits);
  const int32_t denom = (1 << (half - 1)) - 1;
  const int32_t mask = (1 << half) - 1;
  const auto quantize = [denom, mask](double coordinate) noexcept -> int32_t {
    const double clamped = std::min(1.0, std::max(-1.0, coordinate));
    return static_cast<int32_t>(std::lround(clamped * denom)) & mask;
  };
  return (quantize(u) << half) | quantize(v);
}

Eigen::Vector3d octahedral_decode(int32_t index, BitDepth bits) noexcept {
  const int half = half_bits(bits);
  const int32_t full = 1 << half;
  const int32_t mask = full - 1;
  const int32_t sign_bit = 1 << (half - 1);
  const double denom = static_cast<double>((1 << (half - 1)) - 1);

  // Extract two signed two's-complement sub-fields (high half = first coordinate).
  const auto sign_extend = [full, sign_bit](int32_t field) noexcept -> int32_t {
    return (field >= sign_bit) ? field - full : field;
  };
  const int32_t e0 = sign_extend((index >> half) & mask);
  const int32_t e1 = sign_extend(index & mask);

  const double u = std::min(1.0, std::max(-1.0, static_cast<double>(e0) / denom));
  const double v = std::min(1.0, std::max(-1.0, static_cast<double>(e1) / denom));

  Eigen::Vector3d n(u, v, 1.0 - std::fabs(u) - std::fabs(v));
  if (n.z() < 0.0) {
    const Eigen::Vector2d folded = octahedral_fold(n.x(), n.y());
    n.x() = folded.x();
    n.y() = folded.y();
  }
  n.normalize();
  return n;
}

/* ************************************************************************ */
/*                  Spherical-Fibonacci lattice (decode)                  */
/*                       (Keinert et al. 2015)                            */
/* ************************************************************************ */

Eigen::Vector3d fibonacci_decode(int32_t index, BitDepth bits) noexcept {
  const double lattice_size = fibonacci_lattice_size(bits);
  // The stored word is an unsigned lattice index over the whole 2^bits range.
  const double id = static_cast<double>(static_cast<uint32_t>(index));
  const double m = 1.0 - 1.0 / lattice_size;
  const double phi = 2.0 * Math::pi * fract(id * golden_ratio);
  const double cos_theta = m - 2.0 * id / lattice_size;
  const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
  Eigen::Vector3d n(std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta);
  n.normalize();
  return n;
}

/* ************************************************************************ */
/*                      Method-parameterised dispatch                     */
/* ************************************************************************ */

Eigen::Vector3d sphere_decode(int32_t index, Scheme scheme, BitDepth bits) noexcept {
  switch (scheme) {
  case Scheme::Fibonacci:
    return fibonacci_decode(index, bits);
  case Scheme::Octahedral:
  default:
    return octahedral_decode(index, bits);
  }
}

int32_t sphere_encode(const Eigen::Vector3d &unit, Scheme scheme, BitDepth bits) {
  switch (scheme) {
  case Scheme::Fibonacci:
    throw Exception("the spherical-Fibonacci .qfib method is supported for reading only,"
                    " not for writing");
  case Scheme::Octahedral:
  default:
    return octahedral_encode(unit, bits);
  }
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

template <class V>
Compressed<V> compress(const Streamline<V> &tck, Scheme scheme, BitDepth bits, double ratio, double step_tol) {
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
    const int32_t index = sphere_encode(inverse_mapping(tangent, axis, ratio), scheme, bits);
    out.indices.push_back(index);
    axis = mapping(sphere_decode(index, scheme, bits), axis, ratio);
    current += stepsize * axis;
  }
  return out;
}

template <class V> Streamline<V> decompress(const Compressed<V> &cfiber, Scheme scheme, BitDepth bits, double ratio) {
  Streamline<V> tck;
  tck.reserve(cfiber.indices.size() + 2);
  tck.push_back(cfiber.first);
  tck.push_back(cfiber.second);

  const Eigen::Vector3d origin = cfiber.first.template cast<double>();
  const Eigen::Vector3d second = cfiber.second.template cast<double>();
  const double stepsize = (second - origin).norm();
  Eigen::Vector3d axis = (second - origin).normalized();

  for (const int32_t index : cfiber.indices) {
    const Eigen::Vector3d tangent = mapping(sphere_decode(index, scheme, bits), axis, ratio);
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
template Compressed<float> compress<float>(const Streamline<float> &, Scheme, BitDepth, double, double);
template Compressed<double> compress<double>(const Streamline<double> &, Scheme, BitDepth, double, double);
template Streamline<float> decompress<float>(const Compressed<float> &, Scheme, BitDepth, double);
template Streamline<double> decompress<double>(const Compressed<double> &, Scheme, BitDepth, double);

} // namespace MR::DWI::Tractography::Formats::QFibCodec
