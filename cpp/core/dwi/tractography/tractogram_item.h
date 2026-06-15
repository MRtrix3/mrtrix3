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

#include <cstdint>
#include <vector>

#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

//! \brief The composite item that flows through the tractography thread queue (§2.1).
/*! This is the unit of work passed between a Reader (source), the
 * multi-threaded worker functors (pipe), and a Writer (sink). It bundles the
 * streamline vertices (in processing precision, carrying their own weight and
 * DataIndex) with the per-streamline (dps) and per-vertex (dpv) sidecar
 * payloads, each an ordinal-indexed std::vector against the owning
 * Tractogram's field registry (§2.5/§2.7).
 *
 * \par Sidecar payload (Stage 10)
 * The per-streamline (dps) and per-vertex (dpv) sidecar payloads are
 * ordinal-indexed std::vectors of the DPSValue / DPVValue std::variant types
 * (sidecar_value.h, §2.2). Each slot's ordinal is assigned by the owning
 * Tractogram's field registry (field_registry.h, §2.5); the no-sidecar common
 * case is two empty vectors (a fast path). Group membership is reserved as a
 * compact set of group ordinals (§2.1) for Stage 17; declared now so the
 * carry-with-streamline model (D2) extends to groups without a later signature
 * change.
 *
 * \par The privileged "weight" field (D3 / §2.1)
 * The SIFT2-style per-streamline weight is NOT a generic \c dps field: it is a
 * member of the embedded Streamline (Streamline::weight) — its single source of
 * truth — so the many commands that read tck.weight keep behaving identically. A
 * weight enters or leaves Streamline::weight ONLY when the user names a source /
 * destination on the command line (a standalone scalar file, or a named field of a
 * tractogram dataset; dwi/tractography/weights.h). It is never inferred: a generic
 * \c dps field that merely happens to be named "weights" stays an ordinary
 * pass-through field, and is treated as the streamline weight only when explicitly
 * designated. The \c dps vector therefore holds the additional per-streamline
 * fields, never a duplicate of the weight. */
template <class ValueType = float> class TractogramItem {
public:
  using value_type = ValueType;

  TractogramItem() = default;
  TractogramItem(const Streamline<ValueType> &streamline) : streamline(streamline) {}
  TractogramItem(Streamline<ValueType> &&streamline) : streamline(std::move(streamline)) {}

  //! reset the item to an empty state, ready for the next read
  void clear() {
    streamline.clear();
    dps.clear();
    dpv.clear();
    groups.clear();
  }

  //! the ordinal of this streamline within the dataset (delegates to Streamline)
  size_t get_index() const { return streamline.get_index(); }
  void set_index(const size_t i) { streamline.set_index(i); }

  //! the streamline vertices, weight and ordering index
  Streamline<ValueType> streamline;

  //! per-streamline sidecar fields, ordinal-indexed against the dps registry (§2.5)
  std::vector<DPSValue> dps;
  //! per-vertex sidecar fields, ordinal-indexed against the dpv registry (§2.5)
  std::vector<DPVValue> dpv;
  //! \brief reserved group-membership slot (§2.1; populated in Stage 17).
  /*! The set of group ordinals this streamline belongs to, kept sorted/unique;
   * empty until grouping is implemented. */
  std::vector<uint32_t> groups;
};

} // namespace MR::DWI::Tractography
