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

#include <array>
#include <cstddef>
#include <limits>
#include <utility>

#include "math/hermite.h"
#include "types.h"

namespace MR::DWI::Tractography {

// Base class for storing an index alongside either streamline vertex or track scalar data
//
class DataIndex {
public:
  static constexpr size_t invalid = std::numeric_limits<size_t>::max();
  DataIndex() : index(invalid) {}
  DataIndex(const size_t i) : index(i) {}
  DataIndex(const DataIndex &i) : index(i.index) {}
  DataIndex(DataIndex &&i) : index(i.index) { i.index = invalid; }
  DataIndex &operator=(const DataIndex &i) {
    index = i.index;
    return *this;
  }
  DataIndex &operator=(DataIndex &&i) {
    index = i.index;
    i.index = invalid;
    return *this;
  }
  void set_index(const size_t i) { index = i; }
  size_t get_index() const { return index; }
  void clear() { index = invalid; }
  bool operator<(const DataIndex &i) const { return index < i.index; }

private:
  size_t index;
};

// A class for track scalars
template <typename ValueType = float> class TrackScalar : public std::vector<ValueType>, public DataIndex {
public:
  using value_type = ValueType;
  using std::vector<ValueType>::vector;
  TrackScalar() = default;
  TrackScalar(const TrackScalar &) = default;
  TrackScalar(TrackScalar &&that) : std::vector<value_type>(std::move(that)), DataIndex(std::move(that)) {}
  TrackScalar &operator=(const TrackScalar &that) = default;
  void clear() {
    std::vector<ValueType>::clear();
    DataIndex::clear();
  }
};

template <typename ValueType = float>
class Streamline : public std::vector<Eigen::Matrix<ValueType, 3, 1>>, public DataIndex {
public:
  using point_type = Eigen::Matrix<ValueType, 3, 1>;
  using tangent_type = point_type;
  using value_type = ValueType;

  Streamline() : weight(1.0f) {}

  Streamline(size_t size) : std::vector<point_type>(size), weight(value_type(1.0)) {}

  Streamline(size_t size, const point_type &fill) : std::vector<point_type>(size, fill), weight(value_type(1.0)) {}

  Streamline(const Streamline &) = default;
  Streamline &operator=(const Streamline &that) = default;

  Streamline(Streamline &&that)
      : std::vector<point_type>(std::move(static_cast<std::vector<point_type> &&>(that))),
        DataIndex(std::move(static_cast<DataIndex &&>(that))),
        weight(that.weight) {
    that.weight = std::numeric_limits<float>::quiet_NaN();
  }

  Streamline(const std::vector<point_type> &tck) : std::vector<point_type>(tck), DataIndex(), weight(1.0) {}

  Streamline &operator=(Streamline &&that) {
    std::vector<point_type>::operator=(std::move(static_cast<std::vector<point_type> &&>(that)));
    DataIndex::operator=(std::move(static_cast<DataIndex &&>(that)));
    weight = that.weight;
    that.weight = 0.0f;
    return *this;
  }

  void clear() {
    std::vector<point_type>::clear();
    DataIndex::clear();
    weight = 1.0;
  }

  float weight;
};

//! Geometric model under which a streamline's length is measured.
enum class LengthMethod {
  CHORD, //!< Sum of Euclidean distances between consecutive vertices (the piecewise-linear polyline).
  SPLINE //!< Arc length of the interpolating cubic Catmull-Rom spline, by Gauss-Legendre quadrature.
};

//! Arc length of the cubic Catmull-Rom spline interpolating \a tck, via Gauss-Legendre quadrature.
/*! Each segment of the spline is a cubic in the local parameter \c mu, so its speed
 *  \f$\lVert dP/d\mu \rVert\f$ is the square root of a quartic polynomial; the segment arc length
 *  \f$\int_0^1 \lVert dP/d\mu \rVert\, d\mu\f$ has no elementary closed form (it is elliptic). Because
 *  the per-segment speed is smooth for non-degenerate vertices, a fixed low-order Gauss-Legendre rule
 *  integrates it to well below tractography precision at O(1) cost per segment, rather than realising
 *  and summing a dense polyline. Only the parametric tangent is needed, so \c Math::Hermite is run in
 *  derivative-only mode. The first and last segments rely on the reflected ghost vertices
 *  \f$2 P_0 - P_1\f$ and \f$2 P_{n-1} - P_{n-2}\f$, matching the canonical Catmull-Rom convention of
 *  \c SplineView (dwi/tractography/spline.h); tension is fixed at zero (uniform Catmull-Rom). */
template <typename PointType> typename PointType::Scalar spline_length(const std::vector<PointType> &tck) {
  using Scalar = typename PointType::Scalar;
  const size_t num_vertices = tck.size();
  if (num_vertices < 2)
    return Scalar(0);
  // Up-sampling ratio for the quadrature: a fixed five-node Gauss-Legendre rule per segment.
  //   A rule of order m is exact for polynomials up to degree 2m-1; with the integrand being the
  //   square root of a quartic, five nodes (exact to degree nine) drive the per-segment relative
  //   error well below single-precision streamline resolution for any realistic curvature, while
  //   keeping a deterministic, curvature-independent cost. The abscissae and weights below are the
  //   canonical [-1,1] Gauss-Legendre nodes mapped onto [0,1] (x -> (x+1)/2, w -> w/2); they are
  //   specific to order five and must be regenerated if the order is changed.
  constexpr size_t order = 5;
  static constexpr std::array<default_type, order> nodes = {
      0.046910077030668004, 0.230765344947158430, 0.500000000000000000, 0.769234655052841570, 0.953089922969331996};
  static constexpr std::array<default_type, order> weights = {
      0.118463442528094544, 0.239314335249683234, 0.284444444444444444, 0.239314335249683234, 0.118463442528094544};
  const PointType front_ghost = (tck[0] * Scalar(2)) - tck[1];
  const PointType back_ghost = (tck[num_vertices - 1] * Scalar(2)) - tck[num_vertices - 2];
  Math::Hermite<Scalar> hermite(Scalar(0), Math::SplineProcessingType::Derivative);
  default_type total = 0.0;
  for (size_t segment = 0; segment + 1 != num_vertices; ++segment) {
    const PointType &p0 = (segment == 0) ? front_ghost : tck[segment - 1];
    const PointType &p1 = tck[segment];
    const PointType &p2 = tck[segment + 1];
    const PointType &p3 = (segment + 2 < num_vertices) ? tck[segment + 2] : back_ghost;
    default_type segment_length = 0.0;
    for (size_t node = 0; node != order; ++node) {
      hermite.set(Scalar(nodes[node]));
      segment_length += weights[node] * static_cast<default_type>(hermite.derivative(p0, p1, p2, p3).norm());
    }
    total += segment_length;
  }
  return Scalar(total);
}

//! Geometric length of a streamline under the chosen \a method (see \c LengthMethod).
template <typename PointType>
typename PointType::Scalar length(const std::vector<PointType> &tck, const LengthMethod method) {
  using Scalar = typename PointType::Scalar;
  if (tck.empty())
    return std::numeric_limits<Scalar>::quiet_NaN();
  switch (method) {
  case LengthMethod::CHORD: {
    Scalar value = Scalar(0);
    for (size_t i = 1; i != tck.size(); ++i)
      value += (tck[i] - tck[i - 1]).norm();
    return value;
  }
  case LengthMethod::SPLINE:
    return spline_length(tck);
  }
  assert(false);
  return std::numeric_limits<Scalar>::quiet_NaN();
}

template <typename PointType> PointType tangent(const std::vector<PointType> &tck, const size_t index) {
  assert(index < tck.size());
  if (tck.size() < 2)
    return PointType::Constant(std::numeric_limits<typename PointType::Scalar>::quiet_NaN());
  if (!index)
    return (tck[1] - tck[0]).normalized();
  if (index == tck.size() - 1)
    return (tck[index] - tck[index - 1]).normalized();
  const PointType offset_prev = tck[index] - tck[index - 1];
  const PointType offset_next = tck[index + 1] - tck[index];
  const typename PointType::Scalar dist_prev = offset_prev.norm();
  const typename PointType::Scalar dist_next = offset_next.norm();
  if (dist_prev == typename PointType::Scalar(0)) {
    return (dist_next == typename PointType::Scalar(0)
                ? PointType::Constant(std::numeric_limits<typename PointType::Scalar>::quiet_NaN())
                : offset_next.normalized());
  } else if (dist_next == typename PointType::Scalar(0)) {
    return offset_prev.normalized();
  }
  // Greater weight given to the shorter step
  return (dist_next * offset_prev.normalized() + dist_prev * offset_next.normalized()).normalized();
}

} // namespace MR::DWI::Tractography
