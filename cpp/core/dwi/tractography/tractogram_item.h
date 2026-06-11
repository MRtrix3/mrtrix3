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
 * \par The reserved "weight" field (D3 / §2.1)
 * The SIFT2-style per-streamline weight is NOT stored in \c dps: it remains a
 * member of the embedded Streamline (Streamline::weight) — the single source of
 * truth — so the many commands that read tck.weight keep compiling and behaving
 * identically. A "weight" descriptor may still appear in the field registry
 * (ordinal-reserved), but its value is sourced from / written to
 * streamline.weight, never duplicated into the \c dps vector. The \c dps vector
 * holds only the additional per-streamline fields. */
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
