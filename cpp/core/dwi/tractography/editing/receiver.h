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

#include <cinttypes>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "progressbar.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/selection_dps.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "dwi/tractography/weights.h"

namespace MR::DWI::Tractography::Editing {

class Receiver {

public:
  Receiver(const std::filesystem::path &path,
           const Properties &properties,
           const size_t n,
           const size_t s,
           const WeightOutput &weight_output,
           const bool may_fragment)
      : output(Tractogram<float>::create(path, properties, weight_output.registry)),
        number(n),
        skip(s),
        // Need to use local counts instead of writer class members due to track cropping
        count(0),
        total_count(0),
        // A vertex mask or a per-vertex (dpv) field threshold can fragment one input
        //   streamline into several outputs, in which case the segment tally is shown.
        crop(may_fragment),
        segments(0),
        progress(std::string("       0 read,        0 written") + (crop ? ",        0 segments" : "")) {
    // Route the privileged streamline weight to its resolved destination (an
    //   external file, an embedded field, or — per the provenance default —
    //   suppressed). tckedit's output registry is otherwise empty (generic dps/dpv
    //   are not propagated), so the weight is never double-carried.
    apply_weight_output(output, weight_output);
  }

  ~Receiver() {
    // Use set_text() rather than update() here to force update of the text before progress goes out of scope
    progress.set_text(std::string(printf("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count)) +
                      (crop ? printf(", %8" PRIu64 " segments", segments) : ""));
    if (number && (count != number))
      WARN("User requested " + str(number) + " streamlines, but only " + str(count) + " were written to file");
    if (selection_dps_path.has_value())
      write_selection_dps(*selection_dps_path, selection_dps_values);
  }

  //! \brief request an embedded per-output-streamline selection dps field (step 3).
  /*! When set, the Receiver records a "selected" value (1) for every streamline
   * written to the output — including each fragment produced when vertex-masking
   * fragments a single input streamline into several output streamlines — and
   * writes them as a standalone per-streamline dps sidecar on destruction. */
  void set_selection_dps_path(const std::filesystem::path &path) { selection_dps_path = path; }

  //! \brief no-mask path: consume one item per input streamline.
  bool operator()(const TractogramItem<> &);
  //! \brief mask path: consume the fragment items of one input streamline.
  /*! The fragments deliberately share an index; this method must not rely on
   * per-fragment index values or ordering. */
  bool operator()(const std::vector<TractogramItem<>> &);

private:
  Tractogram<float> output;
  const uint64_t number;
  uint64_t skip;
  uint64_t count, total_count;
  bool crop;
  uint64_t segments;
  ProgressBar progress;
  std::optional<std::filesystem::path> selection_dps_path;
  std::vector<uint8_t> selection_dps_values;

  //! \brief record a selection value for one output streamline (a fragment included).
  void record_selection() {
    if (selection_dps_path.has_value())
      selection_dps_values.push_back(1);
  }
};

} // namespace MR::DWI::Tractography::Editing
