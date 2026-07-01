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

#include <cstddef>
#include <string>
#include <vector>

namespace MR::DWI::Tractography::Editing {

//! \brief Whether a threshold bounds a scalar field value from below or above.
enum class Bound {
  Min, //!< a lower bound: the value is retained iff it is >= the threshold
  Max  //!< an upper bound: the value is retained iff it is <= the threshold
};

//! \brief One min-or-max threshold applied to a named scalar sidecar field.
/*! A single resolved "-dps_min / -dps_max / -dpv_min / -dpv_max" request. The
 * field name has been resolved against the input tractogram's field registry to a
 * role-local ordinal, so the worker indexes the per-item payload directly
 * (TractogramItem::dps for a dps filter, TractogramItem::dpv for a dpv filter). */
struct FieldFilter {
  size_t ordinal;   //!< role-local ordinal of the field in the input registry (§2.5)
  Bound bound;      //!< whether \c value is a lower (Min) or upper (Max) bound
  float value;      //!< the threshold value
  std::string name; //!< the field name (retained for diagnostics only)
};

//! \brief The resolved per-streamline (dps) and per-vertex (dpv) field filters.
/*! A dps filter discards a whole streamline whose scalar field value violates the
 * threshold; a dpv filter discards individual vertices, which — exactly as a vertex
 * mask does — may fragment one input streamline into several output streamlines. */
struct FieldFilters {
  std::vector<FieldFilter> dps;
  std::vector<FieldFilter> dpv;
  bool empty() const { return dps.empty() && dpv.empty(); }
};

} // namespace MR::DWI::Tractography::Editing
