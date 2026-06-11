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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "datatype.h"

namespace MR::DWI::Tractography {

//! \brief The role a sidecar field plays relative to the streamlines (§1.1).
enum class FieldRole {
  DPS, //!< data per streamline (one value/row per streamline)
  DPV, //!< data per vertex (one value/row per vertex)
  DPG  //!< data per group (metadata attached to a named group)
};

//! \brief The provenance of a sidecar field.
enum class FieldSource {
  Internal, //!< the field is carried within this tractography dataset
  External  //!< the field is sourced from a separate sidecar file
};

//! \brief Descriptor for a single sidecar field (§2.5).
/*! Records a field's name, role, native on-disk datatype, column count and
 * provenance. The registry assigns each descriptor a stable ordinal that
 * indexes the per-item payload vectors of TractogramItem (§2.1). */
struct FieldDescriptor {
  std::string name;
  FieldRole role;
  DataType dtype;
  size_t columns; //!< the field's column count M (§2.2); 1 for a scalar field
  FieldSource source;
};

//! \brief The sidecar field registry owned by a Tractogram (§2.5).
/*! When a tractogram is opened (read) or created (write), its handler
 * enumerates the sidecar fields it carries and registers a FieldDescriptor for
 * each. The registry assigns a stable ordinal per field and maps name →
 * ordinal, so that per-item payloads (§2.1) are std::vectors indexed by that
 * ordinal rather than maps keyed by name. A pipeline holds two registries — one
 * per input/output Tractogram — with a name-based pass-through map between them
 * (§2.7).
 *
 * \par Stage 1 scope
 * The ".tck" format carries no internal sidecar fields, so a Stage-1 registry
 * is always empty. The type is introduced now, with the §2.5 shape, so that
 * Tractogram owns it from the outset and the sidecar machinery of later stages
 * slots in without changing Tractogram's interface. */
class FieldRegistry {
public:
  //! \brief register a field, returning its assigned ordinal.
  size_t add(const FieldDescriptor &descriptor) {
    const size_t ordinal = descriptors.size();
    descriptors.push_back(descriptor);
    return ordinal;
  }

  //! \brief the number of registered fields.
  size_t size() const { return descriptors.size(); }
  bool empty() const { return descriptors.empty(); }

  //! \brief the descriptor at the given ordinal.
  const FieldDescriptor &operator[](const size_t ordinal) const { return descriptors[ordinal]; }

  //! \brief resolve a field name to its ordinal, if present.
  std::optional<size_t> ordinal(std::string_view name) const {
    for (size_t i = 0; i != descriptors.size(); ++i) {
      if (descriptors[i].name == name)
        return i;
    }
    return std::nullopt;
  }

private:
  std::vector<FieldDescriptor> descriptors;
};

} // namespace MR::DWI::Tractography
