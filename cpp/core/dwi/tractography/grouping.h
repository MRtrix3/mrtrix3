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
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "datatype.h"
#include "exception.h"

#include "dwi/tractography/sidecar_value.h"

namespace MR::DWI::Tractography {

//! \brief The mutual in-memory representation of streamline grouping (§2.3 / D6).
/*! A Grouping encapsulates the single data model shared by every MRtrix3
 * facility that partitions or labels streamlines into named subsets: TRX
 * `groups`/`dpg`, and connectome node assignments. It is a strict superset of
 * both:
 *   - a TRX `groups/<name>.uint32` table is a named group of streamline indices;
 *   - a TRX `dpg/<group>/<field>` array is per-group metadata on that group;
 *   - a connectome `assignments_pairs` entry `(n1, n2)` is a group named
 *     `"<n1>-<n2>"` (one per realised edge);
 *   - a connectome `assignments_single` entry is a group named per node;
 *   - a connectome `assignments_lists` entry yields multi-membership (a
 *     streamline in many groups);
 *   - an `Exemplar`'s per-edge data is a dpg field on the corresponding edge
 *     group.
 *
 * \par Structure
 * Two ordered tables, keyed by group name so that the on-disk order is preserved
 * across a round-trip:
 *   - `indices`: name → the (uint32) streamline indices belonging to the group;
 *   - `dpg`: name → (field name → ScalarOrVector value in the field's native
 *     dtype). The DPSValue variant (sidecar_value.h, §2.2) is reused so that
 *     per-group metadata keeps its native on-disk element type (D7), exactly as
 *     the per-streamline dps payload does.
 *
 * \par Invariants
 * Every index satisfies `0 <= index < num_streamlines()`; groups may overlap and
 * a streamline may belong to many groups; dpg fields need not be present on every
 * group (mirroring the TRX spec). The streamline-count bound is validated by
 * validate() once the total streamline count is known. */
class Grouping {
public:
  //! \brief the per-group metadata table (TRX dpg) for one group.
  /*! Maps a dpg field name to its value, kept in its native on-disk dtype via the
   * DPSValue variant (a ScalarOrVector<T> per §2.2). Ordered by field name so the
   * on-disk member order round-trips. */
  using DPGFields = std::map<std::string, DPSValue>;

  Grouping() = default;

  //! \brief whether any groups are defined.
  bool empty() const { return indices_.empty(); }
  //! \brief the number of defined groups.
  size_t size() const { return indices_.size(); }

  //! \brief whether a group of the given name exists.
  bool has_group(std::string_view name) const { return indices_.find(std::string(name)) != indices_.end(); }

  //! \brief define (or replace) a group's streamline-index membership.
  /*! The supplied indices are stored verbatim (not sorted or de-duplicated): the
   * TRX spec permits a group to list indices in any order, and connectome
   * assignment order is preserved. Use add_member() to append incrementally. */
  void set_group(std::string_view name, std::vector<uint32_t> members) {
    indices_[std::string(name)] = std::move(members);
  }

  //! \brief append a single streamline index to a (possibly new) group.
  /*! Realises multi-membership: calling add_member for the same \a index against
   * several group names places that streamline in all of them. */
  void add_member(std::string_view name, const uint32_t index) { indices_[std::string(name)].push_back(index); }

  //! \brief the streamline indices of the named group; throws if absent.
  const std::vector<uint32_t> &members(std::string_view name) const {
    const auto it = indices_.find(std::string(name));
    if (it == indices_.end())
      throw Exception("grouping has no group named \"" + std::string(name) + "\"");
    return it->second;
  }

  //! \brief iteration over (name → indices) in group-name order.
  auto begin() const { return indices_.begin(); }
  auto end() const { return indices_.end(); }

  //! \brief the ordered group-name → indices table (TRX groups).
  const std::map<std::string, std::vector<uint32_t>> &groups() const { return indices_; }

  //! \brief set a per-group (dpg) metadata field in its native dtype.
  /*! The group need not already have a membership table; a dpg field may be
   * attached to any named group. */
  void set_dpg(std::string_view group, std::string_view field, DPSValue value) {
    dpg_[std::string(group)][std::string(field)] = std::move(value);
  }

  //! \brief the dpg field value for a group, if present.
  const DPSValue *get_dpg(std::string_view group, std::string_view field) const {
    const auto g = dpg_.find(std::string(group));
    if (g == dpg_.end())
      return nullptr;
    const auto f = g->second.find(std::string(field));
    if (f == g->second.end())
      return nullptr;
    return &f->second;
  }

  //! \brief the dpg field table for a group (field name → value), if present.
  const DPGFields *dpg_fields(std::string_view group) const {
    const auto g = dpg_.find(std::string(group));
    return (g == dpg_.end()) ? nullptr : &g->second;
  }

  //! \brief the whole ordered dpg table (group → field → value).
  const std::map<std::string, DPGFields> &dpg() const { return dpg_; }

  //! \brief validate the membership invariant against a known streamline count.
  /*! Throws if any index is out of [0, num_streamlines). Call once the total
   * streamline count is established (e.g. after reading a TRX header, or after
   * importing connectome assignments). */
  void validate(const size_t num_streamlines) const {
    for (const auto &group : indices_) {
      for (const uint32_t index : group.second) {
        if (static_cast<size_t>(index) >= num_streamlines)
          throw Exception("grouping: streamline index " + str(index) + " in group \"" + group.first +
                          "\" is out of range (dataset has " + str(num_streamlines) + " streamlines)");
      }
    }
  }

  //! \brief remove all groups and dpg metadata.
  void clear() {
    indices_.clear();
    dpg_.clear();
  }

private:
  //! ordered (group name → streamline indices) table — the TRX `groups`
  std::map<std::string, std::vector<uint32_t>> indices_;
  //! ordered (group name → (field name → native-dtype value)) table — TRX `dpg`
  std::map<std::string, DPGFields> dpg_;
};

} // namespace MR::DWI::Tractography
