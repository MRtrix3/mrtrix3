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

#include "dwi/tractography/grouping.h"
#include "dwi/tractography/sidecar_value.h"

#include <gtest/gtest.h>

#include <vector>

using MR::DWI::Tractography::Grouping;
using MR::DWI::Tractography::make_dps;
using MR::DWI::Tractography::ScalarOrVector;

namespace {

//! \brief A group's index membership round-trips exactly as set.
TEST(Grouping, SetAndReadMembers) {
  Grouping grouping;
  grouping.set_group("AF_L", {0, 2, 5, 9});
  ASSERT_TRUE(grouping.has_group("AF_L"));
  EXPECT_EQ(grouping.size(), 1u);
  const std::vector<uint32_t> expected{0, 2, 5, 9};
  EXPECT_EQ(grouping.members("AF_L"), expected);
}

//! \brief Groups may overlap: a streamline can be in several groups at once.
TEST(Grouping, OverlappingMembership) {
  Grouping grouping;
  grouping.set_group("bundle_a", {1, 2, 3});
  grouping.set_group("bundle_b", {2, 3, 4});
  // Streamline 2 and 3 are in both groups.
  const auto &a = grouping.members("bundle_a");
  const auto &b = grouping.members("bundle_b");
  EXPECT_NE(std::find(a.begin(), a.end(), 2u), a.end());
  EXPECT_NE(std::find(b.begin(), b.end(), 2u), b.end());
  EXPECT_NE(std::find(a.begin(), a.end(), 3u), a.end());
  EXPECT_NE(std::find(b.begin(), b.end(), 3u), b.end());
}

//! \brief Incremental multi-membership: one streamline added to many groups.
TEST(Grouping, IncrementalMultiMembership) {
  Grouping grouping;
  // Streamline 7 connects three nodes (a list assignment).
  grouping.add_member("1-3", 7);
  grouping.add_member("3-5", 7);
  grouping.add_member("1-5", 7);
  EXPECT_EQ(grouping.size(), 3u);
  EXPECT_EQ(grouping.members("1-3").front(), 7u);
  EXPECT_EQ(grouping.members("3-5").front(), 7u);
  EXPECT_EQ(grouping.members("1-5").front(), 7u);
}

//! \brief Group-name ordering is preserved (an ordered map keyed by name).
TEST(Grouping, OrderedByName) {
  Grouping grouping;
  grouping.set_group("zeta", {0});
  grouping.set_group("alpha", {1});
  grouping.set_group("mu", {2});
  std::vector<std::string> names;
  for (const auto &group : grouping)
    names.push_back(group.first);
  const std::vector<std::string> expected{"alpha", "mu", "zeta"};
  EXPECT_EQ(names, expected);
}

//! \brief dpg get/set: per-group metadata in its native dtype.
TEST(Grouping, DPGGetSet) {
  Grouping grouping;
  grouping.set_group("CC", {0, 1, 2});

  // A scalar float16-style mean (stored here as float for simplicity), and a
  //   3-element uint8 colour, mirroring the TRX dpg example.
  ScalarOrVector<float> mean_fa(1, 1);
  mean_fa(0, 0) = 0.42f;
  grouping.set_dpg("CC", "mean_fa", make_dps(std::move(mean_fa)));

  ScalarOrVector<uint8_t> colour(1, 3);
  colour(0, 0) = 255;
  colour(0, 1) = 128;
  colour(0, 2) = 0;
  grouping.set_dpg("CC", "colour", make_dps(std::move(colour)));

  const auto *fa = grouping.get_dpg("CC", "mean_fa");
  ASSERT_NE(fa, nullptr);
  ASSERT_TRUE(std::holds_alternative<ScalarOrVector<float>>(*fa));
  EXPECT_FLOAT_EQ(std::get<ScalarOrVector<float>>(*fa).scalar(), 0.42f);

  const auto *col = grouping.get_dpg("CC", "colour");
  ASSERT_NE(col, nullptr);
  ASSERT_TRUE(std::holds_alternative<ScalarOrVector<uint8_t>>(*col));
  const auto &row = std::get<ScalarOrVector<uint8_t>>(*col);
  ASSERT_EQ(row.cols(), 3);
  EXPECT_EQ(row(0, 0), 255);
  EXPECT_EQ(row(0, 1), 128);
  EXPECT_EQ(row(0, 2), 0);
}

//! \brief A dpg field absent from a group reports as not-present.
TEST(Grouping, DPGAbsentField) {
  Grouping grouping;
  grouping.set_group("CST_L", {3, 4});
  ScalarOrVector<uint32_t> volume(1, 1);
  volume(0, 0) = 12345u;
  grouping.set_dpg("CST_L", "volume", make_dps(std::move(volume)));
  // mean_fa is not set on this group (mirrors the TRX spec: not all metadata in
  //   all groups).
  EXPECT_EQ(grouping.get_dpg("CST_L", "mean_fa"), nullptr);
  EXPECT_NE(grouping.get_dpg("CST_L", "volume"), nullptr);
  EXPECT_EQ(grouping.get_dpg("nonexistent", "volume"), nullptr);
}

//! \brief validate() enforces 0 <= index < NB_STREAMLINES.
TEST(Grouping, ValidateBounds) {
  Grouping grouping;
  grouping.set_group("g", {0, 4, 9});
  EXPECT_NO_THROW(grouping.validate(10));
  // index 9 is out of range when there are only 9 streamlines (0..8).
  EXPECT_THROW(grouping.validate(9), MR::Exception);
}

//! \brief Reading an undefined group throws.
TEST(Grouping, MissingGroupThrows) {
  Grouping grouping;
  grouping.set_group("present", {0});
  EXPECT_THROW(grouping.members("absent"), MR::Exception);
}

} // namespace
