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
#include <optional>
#include <vector>

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/grouping.h"

namespace MR::DWI::Tractography::Connectome {

//! \brief The per-streamline node assignments of a connectome (Stage 17, step 4).
/*! This is the in-memory form of the connectome "-out_assignments" text file:
 * line N lists the parcellation node(s) to which streamline N was assigned. The
 * three connectome assignment modes (tck2connectome's matrix.{h,cpp}) all
 * reduce to it:
 *   - assignments_single → one node per streamline (a single-integer line);
 *   - assignments_pairs  → a node pair per streamline (a two-integer line);
 *   - assignments_lists  → an arbitrary node list per streamline (multi-node).
 *
 * Assignments encapsulates this per-streamline representation together with its
 * byte-faithful text import/export, and converts to/from the canonical Grouping
 * structure (§2.3 / D6): an edge "<n1>-<n2>" (or a node "<n>") becomes a named
 * group whose members are the streamline indices assigned to it, realising
 * overlap / multi-membership for list assignments. The text file is preserved
 * verbatim as the import/export interface to Grouping (the byte layout written by
 * load() → save() is unchanged from the historical
 * Connectome::Matrix::write_assignments). */
class Assignments {
public:
  Assignments() = default;

  //! \brief append one streamline's node assignment (in streamline order).
  void add(std::vector<node_t> nodes) { per_streamline.push_back(std::move(nodes)); }

  //! \brief the number of streamlines whose assignments are stored.
  size_t size() const { return per_streamline.size(); }
  bool empty() const { return per_streamline.empty(); }

  //! \brief the node assignment of streamline \a index.
  const std::vector<node_t> &operator[](const size_t index) const { return per_streamline[index]; }

  //! \brief the highest node index referenced across all streamlines (0 if none).
  node_t max_node() const;

  //! \brief whether every streamline is assigned to exactly two nodes.
  /*! When true the assignments form a pure edge list; connectome2tck takes its
   * faster NodePair path. */
  bool all_pairs() const;

  //! \brief the per-streamline assignments as node pairs (requires all_pairs()).
  std::vector<NodePair> as_pairs() const;

  //! \brief the per-streamline assignments as (sorted) node lists.
  const std::vector<std::vector<node_t>> &as_lists() const { return per_streamline; }

  //! \brief read an "-out_assignments" text file (the import interface).
  /*! Mirrors the historical connectome2tck reader exactly: '#'-introduced
   * comments are stripped, blank lines skipped, and each remaining line parsed as
   * a whitespace-separated node list, in streamline order. */
  static Assignments load(const std::filesystem::path &path);

  //! \brief write the "-out_assignments" text file (the export interface).
  /*! Byte-for-byte identical to the historical
   * Connectome::Matrix::write_assignments output for the equivalent assignments:
   * a leading "# <command history>" comment, then one line per streamline with
   * the assigned node indices space-separated. */
  void save(const std::filesystem::path &path) const;

  //! \brief encode these assignments into the canonical Grouping (§2.3 / D6).
  /*! Each realised edge "<n1>-<n2>" (for a pair), node "<n>" (for a single
   * node), or multi-node membership (for a list) becomes a named group whose
   * members are the streamline indices assigned to it. A streamline assigned to
   * several nodes joins several groups (overlap / multi-membership). Group names
   * are the canonical node / edge names so they round-trip with TRX groups. */
  Grouping to_grouping() const;

private:
  //! per-streamline node assignments, indexed by streamline ordinal
  std::vector<std::vector<node_t>> per_streamline;
};

//! \brief the canonical group name for a single node (e.g. "12").
std::string node_group_name(node_t node);
//! \brief the canonical group name for an edge between two nodes (e.g. "1-2").
/*! The lower node index is written first so an undirected edge has one name. */
std::string edge_group_name(node_t one, node_t two);

} // namespace MR::DWI::Tractography::Connectome
