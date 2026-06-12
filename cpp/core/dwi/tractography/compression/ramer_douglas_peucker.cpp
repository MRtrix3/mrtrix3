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

#include "dwi/tractography/compression/ramer_douglas_peucker.h"

#include <stack>
#include <utility>

#include "math/math.h"

namespace MR::DWI::Tractography::Compression {

namespace {

//! \brief Perpendicular distance from \a point to the segment [\a a, \a b].
/*! Falls back to the point-to-endpoint distance when the segment is degenerate
 * (the two endpoints coincide), so a zero-length chord does not divide by zero. */
template <class ValueType>
ValueType point_to_segment_distance(const Eigen::Matrix<ValueType, 3, 1> &point,
                                    const Eigen::Matrix<ValueType, 3, 1> &a,
                                    const Eigen::Matrix<ValueType, 3, 1> &b) {
  const Eigen::Matrix<ValueType, 3, 1> ab = b - a;
  const ValueType ab_sq = ab.squaredNorm();
  if (ab_sq <= ValueType(0))
    return (point - a).norm();
  // |(point - a) x (b - a)| / |b - a| is the perpendicular distance to the
  //   infinite line through a and b; for an interior RDP vertex this bounds its
  //   deviation from the retained chord.
  return (point - a).cross(ab).norm() / std::sqrt(ab_sq);
}

} // namespace

template <class ValueType>
std::vector<size_t> rdp_retained_indices(const Streamline<ValueType> &tck, const ValueType tolerance_mm) {
  const size_t num_points = tck.size();
  std::vector<size_t> indices;
  if (num_points < 3) {
    indices.reserve(num_points);
    for (size_t i = 0; i != num_points; ++i)
      indices.push_back(i);
    return indices;
  }

  std::vector<bool> keep(num_points, false);
  keep[0] = true;
  keep[num_points - 1] = true;

  // Explicit stack of [first, last] index spans yet to be examined.
  std::stack<std::pair<size_t, size_t>> pending;
  pending.emplace(0, num_points - 1);
  while (!pending.empty()) {
    const std::pair<size_t, size_t> span = pending.top();
    pending.pop();
    const size_t first = span.first;
    const size_t last = span.second;
    if (last <= first + 1)
      continue; // no interior vertex to test

    ValueType max_distance = ValueType(0);
    size_t split = first;
    for (size_t i = first + 1; i != last; ++i) {
      const ValueType distance = point_to_segment_distance<ValueType>(tck[i], tck[first], tck[last]);
      if (distance > max_distance) {
        max_distance = distance;
        split = i;
      }
    }

    // Strict comparison: retain the farthest vertex only when it deviates by more
    //   than the tolerance (tunable for byte-exact interop; see header).
    if (max_distance > tolerance_mm) {
      keep[split] = true;
      pending.emplace(first, split);
      pending.emplace(split, last);
    }
  }

  for (size_t i = 0; i != num_points; ++i) {
    if (keep[i])
      indices.push_back(i);
  }
  return indices;
}

template <class ValueType>
Streamline<ValueType> linearize(const Streamline<ValueType> &tck, const ValueType tolerance_mm) {
  if (tck.size() < 3)
    return tck;
  const std::vector<size_t> indices = rdp_retained_indices<ValueType>(tck, tolerance_mm);
  Streamline<ValueType> out;
  out.set_index(tck.get_index());
  out.weight = tck.weight;
  out.reserve(indices.size());
  for (const size_t i : indices)
    out.push_back(tck[i]);
  return out;
}

template std::vector<size_t> rdp_retained_indices<float>(const Streamline<float> &, float);
template std::vector<size_t> rdp_retained_indices<double>(const Streamline<double> &, double);
template Streamline<float> linearize<float>(const Streamline<float> &, float);
template Streamline<double> linearize<double>(const Streamline<double> &, double);

} // namespace MR::DWI::Tractography::Compression
