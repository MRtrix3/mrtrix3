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
#include "exception.h"

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
 * provenance, plus the role-local ordinal assigned by the registry. The ordinal
 * is the slot of this field within the matching per-item payload vector of
 * TractogramItem (§2.1): a DPS field's ordinal indexes TractogramItem::dps, a
 * DPV field's ordinal indexes TractogramItem::dpv. Ordinals are therefore
 * counted separately per role. */
struct FieldDescriptor {
  std::string name;
  FieldRole role;
  DataType dtype;
  size_t columns; //!< the field's column count M (§2.2); 1 for a scalar field
  FieldSource source;
  size_t ordinal; //!< role-local payload-vector slot (§2.1); set by FieldRegistry::add()
};

//! \brief The sidecar field registry owned by a Tractogram (§2.5).
/*! When a tractogram is opened (read) or created (write), its handler
 * enumerates the sidecar fields it carries and registers a FieldDescriptor for
 * each. The registry assigns a stable, role-local ordinal per field and maps
 * name → descriptor, so that per-item payloads (§2.1) are std::vectors indexed
 * by that ordinal rather than maps keyed by name. The DPS and DPV ordinal spaces
 * are independent (one per TractogramItem payload vector). A pipeline holds two
 * registries — one per input/output Tractogram — reconciled by a name-based
 * pass-through map (the Shared object, §2.7).
 *
 * The empty registry (no sidecar fields, e.g. ".tck") is the common fast path:
 * size()==0, both payload vectors empty. */
class FieldRegistry {
public:
  //! \brief register a field, returning its assigned role-local ordinal.
  /*! The supplied descriptor's role-local ordinal is overwritten with the
   * assigned value (the next free slot for that role). Throws if a field of the
   * same name and role is already registered. */
  size_t add(FieldDescriptor descriptor) {
    if (ordinal(descriptor.name, descriptor.role).has_value())
      throw Exception("duplicate sidecar field \"" + descriptor.name + "\" registered");
    const size_t ordinal = (descriptor.role == FieldRole::DPV) ? dpv_count_ : dps_count_;
    descriptor.ordinal = ordinal;
    if (descriptor.role == FieldRole::DPV)
      ++dpv_count_;
    else
      ++dps_count_;
    descriptors.push_back(std::move(descriptor));
    return ordinal;
  }

  //! \brief the total number of registered fields (across all roles).
  size_t size() const { return descriptors.size(); }
  bool empty() const { return descriptors.empty(); }

  //! \brief the number of registered data-per-streamline (dps) fields.
  size_t dps_count() const { return dps_count_; }
  //! \brief the number of registered data-per-vertex (dpv) fields.
  size_t dpv_count() const { return dpv_count_; }

  //! \brief the descriptor at the given position in registration order.
  const FieldDescriptor &operator[](const size_t position) const { return descriptors[position]; }

  //! \brief iteration over descriptors in registration order.
  auto begin() const { return descriptors.begin(); }
  auto end() const { return descriptors.end(); }

  //! \brief resolve a field name (optionally constrained to a role) to its
  //!   role-local ordinal, if present.
  std::optional<size_t> ordinal(std::string_view name, std::optional<FieldRole> role = std::nullopt) const {
    for (const auto &descriptor : descriptors) {
      if (descriptor.name == name && (!role.has_value() || descriptor.role == *role))
        return descriptor.ordinal;
    }
    return std::nullopt;
  }

  //! \brief the descriptor for a field name+role, if present.
  const FieldDescriptor *find(std::string_view name, const FieldRole role) const {
    for (const auto &descriptor : descriptors) {
      if (descriptor.name == name && descriptor.role == role)
        return &descriptor;
    }
    return nullptr;
  }

  //! \brief a copy of this registry with the named field (of the given role) removed.
  /*! Every surviving field is re-registered in its original order, so each role's
   * remaining fields receive fresh, contiguous role-local ordinals. Used to pull a
   * designated streamline-weight field out of generic per-streamline (dps)
   * pass-through: the weight is routed through Streamline::weight (its single
   * source of truth), never additionally carried as a dps field. If no field of
   * that name+role is present the result is an exact copy. */
  FieldRegistry copy_excluding(std::string_view name, const FieldRole role) const {
    FieldRegistry result;
    for (const auto &descriptor : descriptors) {
      if (descriptor.name == name && descriptor.role == role)
        continue;
      result.add(descriptor);
    }
    return result;
  }

private:
  std::vector<FieldDescriptor> descriptors;
  size_t dps_count_ = 0;
  size_t dpv_count_ = 0;
};

} // namespace MR::DWI::Tractography
