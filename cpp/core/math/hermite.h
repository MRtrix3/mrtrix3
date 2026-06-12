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
#include <limits>

#include "types.h"

namespace MR::Math {

//! Cubic Hermite (tension Catmull-Rom) basis for interpolation through control points.
/*! For a local parameter \a mu in [0,1], the interpolated position on the segment between
 *  control points \c b and \c c is \c w[0]*a + w[1]*b + w[2]*c + w[3]*d, where \c a and \c d
 *  are the preceding and following control points respectively. The tension parameter follows
 *  the canonical Catmull-Rom convention \c t = 0.5*tension.
 *
 *  In addition to the four position weights, \c set() also populates the four derivative weights
 *  \c wd[i] = d(w[i])/d(mu). These are computed unconditionally alongside the position weights:
 *  the extra arithmetic is negligible, callers that only read \c coef() / \c value() are
 *  numerically unaffected, and the position weight expressions are byte-for-byte identical to
 *  the previous implementation. The derivative accessors / evaluators are purely additive. */
template <typename T> class Hermite {
public:
  using value_type = T;

  //! Struct return bundling an interpolated position with its parametric derivative dP/dmu.
  template <class S> struct PositionAndDerivative {
    S position;
    S derivative;
  };

  Hermite(value_type tension = 0.0) : t(T(0.5) * tension) {}

  void set(value_type position) {
    value_type p2 = position * position;
    value_type p3 = position * p2;
    w[0] = (T(0.5) - t) * (T(2.0) * p2 - p3 - position);
    w[1] = T(1.0) + (T(1.5) + t) * p3 - (T(2.5) + t) * p2;
    w[2] = (T(2.0) + T(2.0) * t) * p2 + (T(0.5) - t) * position - (T(1.5) + t) * p3;
    w[3] = (T(0.5) - t) * (p3 - p2);
    // Derivative basis weights wd[i] = d(w[i])/d(mu); analytic d/dmu of the expressions above.
    wd[0] = (T(0.5) - t) * (T(4.0) * position - T(3.0) * p2 - T(1.0));
    wd[1] = T(3.0) * (T(1.5) + t) * p2 - T(2.0) * (T(2.5) + t) * position;
    wd[2] = (T(4.0) + T(4.0) * t) * position + (T(0.5) - t) - T(3.0) * (T(1.5) + t) * p2;
    wd[3] = (T(0.5) - t) * (T(3.0) * p2 - T(2.0) * position);
  }

  value_type coef(size_t i) const { return (w[i]); }

  //! Derivative basis weight \c d(w[i])/d(mu), mirroring coef().
  value_type coef_derivative(size_t i) const { return (wd[i]); }

  template <class S> S value(const S *vals) const { return (value(vals[0], vals[1], vals[2], vals[3])); }
  template <class S> S value(const std::array<S, 4> vals) const { return (value(vals[0], vals[1], vals[2], vals[3])); }

  template <class S> S value(const S &a, const S &b, const S &c, const S &d) const {
    return (w[0] * a + w[1] * b + w[2] * c + w[3] * d);
  }

  //! First derivative of the interpolated position with respect to the interpolation coefficient mu.
  /*! This is the parametric derivative dP/dmu, \b not the arc-length derivative: callers wanting a
   *  unit tangent must \c .normalized() the result, while callers wanting the local parametric speed
   *  use its norm. Evaluated from the same four control points as value() at the mu most recently
   *  passed to set(). */
  template <class S> S derivative(const S *vals) const { return (derivative(vals[0], vals[1], vals[2], vals[3])); }
  template <class S> S derivative(const std::array<S, 4> vals) const {
    return (derivative(vals[0], vals[1], vals[2], vals[3]));
  }
  template <class S> S derivative(const S &a, const S &b, const S &c, const S &d) const {
    return (wd[0] * a + wd[1] * b + wd[2] * c + wd[3] * d);
  }

  //! Interpolated position together with its parametric derivative dP/dmu from one set of control points.
  /*! Returns both quantities in a single pass over the four control points; the derivative is the
   *  parametric tangent dP/dmu (normalise for a unit tangent). */
  template <class S> PositionAndDerivative<S> value_and_derivative(const S *vals) const {
    return (value_and_derivative(vals[0], vals[1], vals[2], vals[3]));
  }
  template <class S> PositionAndDerivative<S> value_and_derivative(const std::array<S, 4> vals) const {
    return (value_and_derivative(vals[0], vals[1], vals[2], vals[3]));
  }
  template <class S>
  PositionAndDerivative<S> value_and_derivative(const S &a, const S &b, const S &c, const S &d) const {
    return {w[0] * a + w[1] * b + w[2] * c + w[3] * d, wd[0] * a + wd[1] * b + wd[2] * c + wd[3] * d};
  }

private:
  std::array<value_type, 4> w;
  std::array<value_type, 4> wd;
  value_type t;
};

} // namespace MR::Math
