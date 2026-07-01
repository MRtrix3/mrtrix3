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

#include <string>
#include <vector>

#include "types.h"

#include "dwi/tractography/editing/field_filter.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"

namespace MR::DWI::Tractography::Editing {

//! \brief tckedit per-streamline worker: filtering and (under masking) cropping.
/*! Provides two call operators selected by the second-pipe element type:
 *   - no-mask: TractogramItem<> in -> TractogramItem<> out (one item, or an empty
 *     item to signal a dropped/excluded streamline);
 *   - mask: TractogramItem<> in -> std::vector<TractogramItem<>> out, yielding one
 *     item per kept contiguous (>= 2 vertex) fragment, replacing the legacy
 *     {NaN,NaN,NaN}-delimited single-streamline stitch.
 *
 * \par Fragment sidecar policy (mask path)
 * Fragments of one input streamline deliberately share that input's index and its
 * per-streamline (dps) payload and weight (all duplicated onto every fragment;
 * downstream must NOT rely on per-fragment index uniqueness or ordering). The
 * per-vertex (dpv) payload is instead split across fragments: each fragment carries
 * exactly the dpv rows for the contiguous original vertices it kept.
 *
 * \par Arbitrary-field thresholds (dps / dpv)
 * In addition to the length / weight thresholds and the include/exclude ROIs, the
 * worker applies user-specified min/max thresholds to named scalar sidecar fields
 * of the input tractogram (FieldFilters; resolved by load_field_filters()). A
 * per-streamline (dps) threshold is a whole-streamline selection criterion (it
 * joins keep()); a per-vertex (dpv) threshold is applied per vertex on the mask
 * path, so — exactly as a vertex mask does — it can fragment one input streamline
 * into several output streamlines. The command therefore routes through the
 * fragmenting (vector-output) pipe whenever a mask OR any dpv threshold is
 * present. */
class Worker {

public:
  Worker(Tractography::Properties &p, const bool inv, const bool end, const FieldFilters &filters)
      : properties(p),
        inverse(inv),
        ends_only(end),
        thresholds(p),
        dps_filters(filters.dps),
        dpv_filters(filters.dpv),
        include_visitation(properties.include, properties.ordered_include) {}

  Worker(const Worker &that)
      : properties(that.properties),
        inverse(that.inverse),
        ends_only(that.ends_only),
        thresholds(that.thresholds),
        dps_filters(that.dps_filters),
        dpv_filters(that.dpv_filters),
        include_visitation(properties.include, properties.ordered_include) {}

  //! \brief no-mask path: filter only; one item out (empty item if excluded).
  bool operator()(TractogramItem<> &, TractogramItem<> &) const;
  //! \brief mask path: filter then crop; one item per kept fragment (empty vector if none).
  bool operator()(TractogramItem<> &, std::vector<TractogramItem<>> &) const;

private:
  const Tractography::Properties &properties;
  const bool inverse, ends_only;

  //! \brief apply length/weight thresholds, dps field thresholds and include/exclude ROIs.
  /*! \returns true if the streamline passes the selection criteria (accounting for
   * -inverse), i.e. it should continue to the cropping / output stage; false if it
   * is to be dropped. Per-vertex masking and dpv field thresholds are NOT applied
   * here (they reshape the vertex axis and are handled per-path by the caller). */
  bool keep(const TractogramItem<> &) const;

  //! \brief whether \a dps satisfies every per-streamline (dps) field threshold.
  bool dps_filters_pass(const std::vector<DPSValue> &dps) const;
  //! \brief whether vertex \a row of \a dpv satisfies every per-vertex (dpv) field threshold.
  bool dpv_filters_pass(const std::vector<DPVValue> &dpv, const size_t row) const;

  class Thresholds {
  public:
    Thresholds(Tractography::Properties &);
    Thresholds(const Thresholds &);
    bool operator()(const Streamline<> &) const;

  private:
    float max_length, min_length;
    float max_weight, min_weight;
    float step_size;
  } thresholds;

  //! per-streamline (dps) field thresholds; applied as whole-streamline criteria
  const std::vector<FieldFilter> dps_filters;
  //! per-vertex (dpv) field thresholds; applied per vertex (may fragment, like a mask)
  const std::vector<FieldFilter> dpv_filters;

  mutable IncludeROIVisitation include_visitation;
};

} // namespace MR::DWI::Tractography::Editing
