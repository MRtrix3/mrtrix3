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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>

#include "dwi/tractography/streamline.h"
#include "math/hermite.h"

namespace MR::DWI::Tractography {

//! Non-owning view of a streamline that exposes the reflected endpoint ghost vertices.
/*! Cubic Hermite (Catmull-Rom) interpolation of the first and last real segment of a
 *  streamline requires one control point beyond each endpoint. The single source of truth
 *  for those ghost vertices is the linear reflection
 *  \f$P_{-1} = 2 P_0 - P_1\f$ (front) and \f$P_n = 2 P_{n-1} - P_{n-2}\f$ (back).
 *
 *  This wrapper stores a reference to a \c Streamline<> (it does not copy or own it) and
 *  precomputes the two ghosts once at construction. \c operator[] accepts a signed index so
 *  that the front ghost is returned at index \c -1, the back ghost at index \c size(), and the
 *  underlying vertex otherwise. Consumers therefore index the spline support
 *  \c {view[k-1], view[k], view[k+1], view[k+2]} for any segment \c k in \c [0, size()-2]
 *  without hand-rolling the reflection or building a padded copy of the streamline.
 *
 *  The reflection is undefined for fewer than two vertices; consumers must guard the
 *  \c size() < 2 case themselves (as they already do, typically by early return) before
 *  constructing a view. Construction asserts this precondition in debug builds and otherwise
 *  reads the (absent) second vertex, so it must not be relied upon to sanitise short inputs.
 *
 *  The class is header-only and trivially inlinable; it sits on hot mapping/resampling paths. */
template <typename ValueType = float> class SplineView {
public:
  using value_type = ValueType;
  using point_type = typename Streamline<ValueType>::point_type;

  //! Construct over a streamline; precomputes the two reflected ghosts. Requires \c tck.size()>=2.
  explicit SplineView(const Streamline<ValueType> &tck)
      : tck(tck), front_ghost((tck[0] * 2.0) - tck[1]), back_ghost((tck[tck.size() - 1] * 2.0) - tck[tck.size() - 2]) {
    assert(tck.size() >= 2);
  }

  //! Vertex access with signed index: front ghost at -1, back ghost at size(), vertex otherwise.
  const point_type &operator[](const ssize_t i) const {
    if (i == static_cast<ssize_t>(-1))
      return front_ghost;
    if (i == static_cast<ssize_t>(tck.size()))
      return back_ghost;
    assert(i >= 0 && i < static_cast<ssize_t>(tck.size()));
    return tck[i];
  }

  size_t size() const { return tck.size(); }
  const point_type &front() const { return tck.front(); }
  const point_type &back() const { return tck.back(); }

  //! The precomputed front ghost vertex \f$2 P_0 - P_1\f$.
  const point_type &front_ghost_vertex() const { return front_ghost; }
  //! The precomputed back ghost vertex \f$2 P_{n-1} - P_{n-2}\f$.
  const point_type &back_ghost_vertex() const { return back_ghost; }

  //! Interpolated position at global parameter \c s = segment + mu (segment in [0, size()-2]).
  /*! Delegates to \c Math::Hermite over the four control points
   *  \c {view[k-1], view[k], view[k+1], view[k+2]} of segment \c k. */
  point_type position(const default_type s, const value_type tension) const {
    const SegmentParameter sp = decompose(s);
    Math::Hermite<value_type> hermite(tension);
    hermite.set(sp.mu);
    return hermite.value(
        operator[](sp.segment - 1), operator[](sp.segment), operator[](sp.segment + 1), operator[](sp.segment + 2));
  }

  //! Parametric tangent dP/dmu at global parameter \c s (normalise for a unit tangent).
  point_type tangent(const default_type s, const value_type tension) const {
    const SegmentParameter sp = decompose(s);
    Math::Hermite<value_type> hermite(tension);
    hermite.set(sp.mu);
    return hermite.derivative(
        operator[](sp.segment - 1), operator[](sp.segment), operator[](sp.segment + 1), operator[](sp.segment + 2));
  }

private:
  const Streamline<ValueType> &tck;
  const point_type front_ghost;
  const point_type back_ghost;

  //! Global spline parameter split into an integer segment index and local mu in [0,1].
  struct SegmentParameter {
    ssize_t segment;
    default_type mu;
  };

  SegmentParameter decompose(const default_type s) const {
    const default_type clamped =
        std::max<default_type>(0.0, std::min<default_type>(s, static_cast<default_type>(tck.size() - 1)));
    ssize_t segment = static_cast<ssize_t>(std::floor(clamped));
    // Keep the final endpoint on the last segment rather than an empty segment beyond it.
    if (segment >= static_cast<ssize_t>(tck.size()) - 1)
      segment = static_cast<ssize_t>(tck.size()) - 2;
    return {segment, clamped - static_cast<default_type>(segment)};
  }
};

} // namespace MR::DWI::Tractography
