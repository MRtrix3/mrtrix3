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

#include <cassert>
#include <cstddef>
#include <vector>

#include "app.h"

#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Resampling {

extern const App::OptionGroup ResampleOption;

class Base;
Base *get_resampler();

using value_type = float;
using point_type = typename Streamline<>::point_type;

// cubic interpolation (tension = 0.0) looks 'bulgy' between control points
constexpr value_type hermite_tension = value_type(0.1);

class Base {
protected:
  Base() {}

public:
  virtual ~Base() {}

  virtual Base *clone() const = 0;
  virtual bool operator()(const Streamline<> &, Streamline<> &) const = 0;
  virtual bool valid() const = 0;

  //! \brief whether this mode retains a subset of the input vertices (§2.7; D8).
  /*! A resampling mode is "vertex-subset preserving" when every output vertex is
   * a verbatim copy of an input vertex (no new positions are interpolated), so
   * that data-per-vertex (dpv) sidecar fields can be carried across by
   * sub-sampling them to the retained vertices (tckresample step 2). The two such
   * modes are downsampling and endpoint extraction. The interpolating modes
   * (fixed step size / Hermite, upsampling, arc/line, fixed number of points)
   * invent new vertex positions, so their dpv fields are dropped; they return
   * false here and do not implement retained_indices(). */
  virtual bool preserves_vertex_subset() const { return false; }

  //! \brief the input-vertex indices retained by a vertex-subset mode.
  /*! \pre preserves_vertex_subset() == true.
   * \returns one index into \a in per output vertex, in output order, so that a
   * caller can sub-sample a parallel dpv field (e.g. a per-vertex ".tsf") to the
   * retained vertices. The default implementation asserts: an interpolating mode
   * must never be asked for retained indices. */
  virtual std::vector<size_t> retained_indices(const Streamline<> &in) const {
    assert(false);
    (void)in;
    return {};
  }
};

template <class Derived> class BaseCRTP : public Base {
protected:
  // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
  BaseCRTP() = default;

public:
  virtual Base *clone() const { return new Derived(static_cast<Derived const &>(*this)); }
};

} // namespace MR::DWI::Tractography::Resampling
