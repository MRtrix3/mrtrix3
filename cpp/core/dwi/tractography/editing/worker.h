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

#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
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
 * exactly the dpv rows for the contiguous original vertices it kept. */
class Worker {

public:
  Worker(Tractography::Properties &p, const bool inv, const bool end)
      : properties(p),
        inverse(inv),
        ends_only(end),
        thresholds(p),
        include_visitation(properties.include, properties.ordered_include) {}

  Worker(const Worker &that)
      : properties(that.properties),
        inverse(that.inverse),
        ends_only(that.ends_only),
        thresholds(that.thresholds),
        include_visitation(properties.include, properties.ordered_include) {}

  //! \brief no-mask path: filter only; one item out (empty item if excluded).
  bool operator()(TractogramItem<> &, TractogramItem<> &) const;
  //! \brief mask path: filter then crop; one item per kept fragment (empty vector if none).
  bool operator()(TractogramItem<> &, std::vector<TractogramItem<>> &) const;

private:
  const Tractography::Properties &properties;
  const bool inverse, ends_only;

  //! \brief apply length/weight thresholds and include/exclude ROIs to one streamline.
  /*! \returns true if the streamline passes the selection criteria (accounting for
   * -inverse), i.e. it should continue to the cropping / output stage; false if it
   * is to be dropped. Masking is NOT applied here (it reshapes the vertex axis and
   * is handled per-path by the caller). */
  bool keep(const Streamline<> &) const;

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

  mutable IncludeROIVisitation include_visitation;
};

} // namespace MR::DWI::Tractography::Editing
