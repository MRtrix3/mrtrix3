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

#include "file/ofstream.h"
#include "file/temp.h"

#include "dwi/tractography/connectome/assignments.h"
#include "dwi/tractography/grouping.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <vector>

using MR::DWI::Tractography::Grouping;
using MR::DWI::Tractography::Connectome::Assignments;
using MR::DWI::Tractography::Connectome::edge_group_name;
using MR::DWI::Tractography::Connectome::node_group_name;

namespace {

//! \brief Pair assignments map to one edge group per realised edge (§2.3).
TEST(ConnectomeAssignments, PairsToGrouping) {
  Assignments a;
  a.add({1, 2}); // streamline 0 → edge 1-2
  a.add({2, 1}); // streamline 1 → edge 1-2 (undirected: same group)
  a.add({3, 5}); // streamline 2 → edge 3-5
  EXPECT_TRUE(a.all_pairs());
  const Grouping g = a.to_grouping();
  ASSERT_TRUE(g.has_group("1-2"));
  ASSERT_TRUE(g.has_group("3-5"));
  const std::vector<uint32_t> edge12{0, 1};
  const std::vector<uint32_t> edge35{2};
  EXPECT_EQ(g.members("1-2"), edge12);
  EXPECT_EQ(g.members("3-5"), edge35);
}

//! \brief Single-node assignments map to per-node groups.
TEST(ConnectomeAssignments, SingleToGrouping) {
  Assignments a;
  a.add({4});
  a.add({4});
  a.add({7});
  EXPECT_FALSE(a.all_pairs());
  const Grouping g = a.to_grouping();
  const std::vector<uint32_t> node4{0, 1};
  const std::vector<uint32_t> node7{2};
  EXPECT_EQ(g.members(node_group_name(4)), node4);
  EXPECT_EQ(g.members(node_group_name(7)), node7);
}

//! \brief List assignments realise overlap / multi-membership across edges.
TEST(ConnectomeAssignments, ListToGroupingMultiMembership) {
  Assignments a;
  a.add({1, 2, 3}); // streamline 0 → edges 1-2, 1-3, 2-3
  const Grouping g = a.to_grouping();
  EXPECT_TRUE(g.has_group("1-2"));
  EXPECT_TRUE(g.has_group("1-3"));
  EXPECT_TRUE(g.has_group("2-3"));
  // Streamline 0 is a member of all three groups simultaneously.
  EXPECT_EQ(g.members("1-2").front(), 0u);
  EXPECT_EQ(g.members("1-3").front(), 0u);
  EXPECT_EQ(g.members("2-3").front(), 0u);
}

//! \brief Edge group names are undirected (lower node first).
TEST(ConnectomeAssignments, EdgeNameUndirected) {
  EXPECT_EQ(edge_group_name(5, 2), "2-5");
  EXPECT_EQ(edge_group_name(2, 5), "2-5");
}

//! \brief A text file round-trips losslessly through load() → save().
/*! The "-out_assignments" text format is the import/export interface to the
 * Grouping; the byte layout (modulo the regenerated command-history comment) is
 * preserved exactly. */
TEST(ConnectomeAssignments, TextRoundTrip) {
  const std::filesystem::path src = MR::File::create_tempfile(0, ".txt");
  {
    MR::File::OFStream stream(src);
    stream << "# some command history\n";
    stream << "1 2\n";
    stream << "3 4\n";
    stream << "5 6 7\n";
  }
  const Assignments a = Assignments::load(src);
  ASSERT_EQ(a.size(), 3u);
  const std::vector<uint32_t> empty;
  EXPECT_EQ(a[0], (std::vector<MR::DWI::Tractography::Connectome::node_t>{1, 2}));
  EXPECT_EQ(a[2], (std::vector<MR::DWI::Tractography::Connectome::node_t>{5, 6, 7}));

  const std::filesystem::path dst = MR::File::create_tempfile(0, ".txt");
  a.save(dst);
  const Assignments b = Assignments::load(dst);
  ASSERT_EQ(b.size(), a.size());
  for (size_t i = 0; i != a.size(); ++i)
    EXPECT_EQ(a[i], b[i]);

  std::error_code ec;
  std::filesystem::remove(src, ec);
  std::filesystem::remove(dst, ec);
}

} // namespace
