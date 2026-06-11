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

#include <vector>

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
 * \par Stage 1 scope
 * Only the \c streamline member is populated. The dps/dpv vectors are present
 * — so the §2.1 shape is fixed and the read/write entry points of Tractogram
 * already take/return this composite — but they are empty and carry no data
 * until the sidecar machinery of later stages. The concrete sidecar value
 * variant types (§2.2, DPSValue / DPVValue) are introduced in Stage 10; for
 * now the vectors hold an opaque placeholder so no later signature churn is
 * required. */
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
  }

  //! the ordinal of this streamline within the dataset (delegates to Streamline)
  size_t get_index() const { return streamline.get_index(); }
  void set_index(const size_t i) { streamline.set_index(i); }

  //! the streamline vertices, weight and ordering index (the only payload in Stage 1)
  Streamline<ValueType> streamline;

  //! \brief placeholder for the data-per-streamline (dps) value type (§2.2).
  /*! Replaced by the DPSValue std::variant in Stage 10. Declared now so the
   * §2.1 composite shape is fixed from Stage 1 and the per-item payload is a
   * std::vector indexed by the dps field ordinal. */
  struct DPSValue {};
  //! \brief placeholder for the data-per-vertex (dpv) value type (§2.2).
  /*! Replaced by the DPVValue std::variant in Stage 10. */
  struct DPVValue {};

  //! per-streamline sidecar fields, ordinal-indexed (empty in Stage 1)
  std::vector<DPSValue> dps;
  //! per-vertex sidecar fields, ordinal-indexed (empty in Stage 1)
  std::vector<DPVValue> dpv;
};

} // namespace MR::DWI::Tractography
