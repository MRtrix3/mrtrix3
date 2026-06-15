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

#include "dwi/tractography/editing/receiver.h"

namespace MR::DWI::Tractography::Editing {

bool Receiver::operator()(const TractogramItem<> &in) {
  auto display_func = [&]() {
    return (printf("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count) +
            (crop ? printf(", %8" PRIu64 " segments", segments) : ""));
  };

  if (number && (count == number))
    return false;

  ++total_count;

  if (in.streamline.empty()) {
    output.note_unexported();
    progress.update(display_func);
    return true;
  }

  if (skip) {
    --skip;
    progress.update(display_func);
    return true;
  }

  output.write(in);
  ++segments;
  record_selection();

  ++count;
  progress.update(display_func);
  return (!(number && (count == number)));
}

bool Receiver::operator()(const std::vector<TractogramItem<>> &fragments) {
  auto display_func = [&]() {
    return (printf("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count) +
            (crop ? printf(", %8" PRIu64 " segments", segments) : ""));
  };

  if (number && (count == number))
    return false;

  ++total_count;

  if (fragments.empty()) {
    output.note_unexported();
    progress.update(display_func);
    return true;
  }

  // -skip operates on whole input streamlines: one decrement per input, not per fragment.
  if (skip) {
    --skip;
    progress.update(display_func);
    return true;
  }

  // Each fragment is an independent output streamline and is assigned its own
  //   selection value (step 3 fragmentation policy). The fragments share an index;
  //   nothing here may rely on per-fragment index values or ordering.
  for (const auto &fragment : fragments) {
    output.write(fragment);
    ++segments;
    record_selection();
  }

  // -number caps the number of input streamlines selected, not fragments.
  ++count;
  progress.update(display_func);
  return (!(number && (count == number)));
}

} // namespace MR::DWI::Tractography::Editing
