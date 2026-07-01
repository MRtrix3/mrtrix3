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

#include <filesystem>
#include <vector>

#include "memory.h"
#include "types.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "dwi/tractography/weights.h"

namespace MR::DWI::Tractography::Editing {

//! \brief tckedit input source: concatenates the input files into one item stream.
/*! Sources from a format-agnostic Tractogram (any registered tractography format),
 * yielding TractogramItem<> so that per-streamline (dps), per-vertex (dpv) and
 * weight payloads flow downstream. The reader is move-assigned as the source
 * advances across input files (Tractogram is factory-only but movable). The
 * explicitly-specified streamline weights (an external file or a named field of the
 * input tractogram) are routed into Streamline::weight; weights are only permitted
 * with a single input file (enforced by the command). */
class Loader {

public:
  Loader(const std::vector<std::filesystem::path> &files)
      : file_list(files),
        dummy_properties(),
        reader(Tractogram<float>::open(file_list[0], dummy_properties)),
        file_index(0) {
    weight_input = register_weight_input(reader, file_list[0]);
  }

  bool operator()(TractogramItem<> &);

  //! \brief the provenance of the input streamline weights (for the output default).
  const WeightInput &weights() const { return weight_input; }

  //! \brief the sidecar field registry of the (first) input tractogram (§2.5).
  /*! Used to resolve the named per-streamline / per-vertex fields of the dps/dpv
   * threshold options against the input dataset. Field-based filtering is
   * restricted to a single input file (enforced by the command), so this registry
   * is authoritative for the ordinals the worker indexes. */
  const FieldRegistry &fields() const { return reader.fields(); }

private:
  const std::vector<std::filesystem::path> &file_list;
  Properties dummy_properties;
  Tractogram<float> reader;
  size_t file_index;
  WeightInput weight_input;
};

bool Loader::operator()(TractogramItem<> &out) {
  out.clear();

  if (reader.read(out))
    return true;

  while (++file_index != file_list.size()) {
    dummy_properties.clear();
    reader = Tractogram<float>::open(file_list[file_index], dummy_properties);
    register_weight_input(reader, file_list[file_index]);
    if (reader.read(out))
      return true;
  }

  return false;
}

} // namespace MR::DWI::Tractography::Editing
