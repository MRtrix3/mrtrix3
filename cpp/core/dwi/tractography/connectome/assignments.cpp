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

#include "dwi/tractography/connectome/assignments.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "app.h"
#include "file/ofstream.h"
#include "mrtrix.h"

namespace MR::DWI::Tractography::Connectome {

std::string node_group_name(const node_t node) { return str(node); }

std::string edge_group_name(const node_t one, const node_t two) {
  const node_t lo = std::min(one, two);
  const node_t hi = std::max(one, two);
  return str(lo) + "-" + str(hi);
}

node_t Assignments::max_node() const {
  node_t maximum = 0;
  for (const auto &nodes : per_streamline)
    for (const node_t n : nodes)
      maximum = std::max(maximum, n);
  return maximum;
}

bool Assignments::all_pairs() const {
  // An empty assignment set is vacuously "all pairs": this matches the historical
  //   connectome2tck logic, whose nonpair_found flag starts false and is only
  //   ever set true by encountering a non-pair line.
  for (const auto &nodes : per_streamline)
    if (nodes.size() != 2)
      return false;
  return true;
}

std::vector<NodePair> Assignments::as_pairs() const {
  std::vector<NodePair> pairs;
  pairs.reserve(per_streamline.size());
  for (const auto &nodes : per_streamline)
    pairs.push_back(NodePair(nodes[0], nodes[1]));
  return pairs;
}

Assignments Assignments::load(const std::filesystem::path &path) {
  Assignments out;
  std::ifstream stream(path);
  std::string line;
  while (std::getline(stream, line)) {
    line = strip(line.substr(0, line.find_first_of('#')));
    if (line.empty())
      continue;
    std::stringstream line_stream(line);
    std::vector<node_t> nodes;
    while (true) {
      node_t n;
      line_stream >> n;
      if (!line_stream)
        break;
      nodes.push_back(n);
    }
    out.per_streamline.push_back(std::move(nodes));
  }
  return out;
}

void Assignments::save(const std::filesystem::path &path) const {
  // Byte-for-byte identical to the historical Matrix::write_assignments output:
  //   a leading command-history comment, then one whitespace-separated node list
  //   per streamline.
  File::OFStream stream(path);
  stream << "# " << App::command_history_string << "\n";
  for (const auto &nodes : per_streamline) {
    if (nodes.empty()) {
      stream << "\n";
      continue;
    }
    stream << str(nodes[0]);
    for (size_t j = 1; j != nodes.size(); ++j)
      stream << " " << str(nodes[j]);
    stream << "\n";
  }
}

Grouping Assignments::to_grouping() const {
  Grouping grouping;
  for (size_t index = 0; index != per_streamline.size(); ++index) {
    const std::vector<node_t> &nodes = per_streamline[index];
    if (nodes.empty())
      continue;
    if (nodes.size() == 2) {
      // An edge: one group named "<lo>-<hi>".
      grouping.add_member(edge_group_name(nodes[0], nodes[1]), static_cast<uint32_t>(index));
    } else if (nodes.size() == 1) {
      // A single-node assignment: one group named "<n>".
      grouping.add_member(node_group_name(nodes[0]), static_cast<uint32_t>(index));
    } else {
      // A multi-node assignment: the streamline joins every pairwise edge group
      //   it realises (overlap / multi-membership), mirroring the connectome
      //   matrix's list accumulation.
      for (size_t i = 0; i != nodes.size(); ++i)
        for (size_t j = i + 1; j != nodes.size(); ++j)
          grouping.add_member(edge_group_name(nodes[i], nodes[j]), static_cast<uint32_t>(index));
    }
  }
  return grouping;
}

} // namespace MR::DWI::Tractography::Connectome
