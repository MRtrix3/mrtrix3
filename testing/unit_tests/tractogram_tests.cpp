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

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/formats/tck.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"

#include <gtest/gtest.h>

#include <Eigen/Core>

#include <cstddef>
#include <filesystem>
#include <vector>

using namespace MR::DWI::Tractography;

namespace {

//! a small set of synthetic streamlines used across the round-trip tests
std::vector<Streamline<float>> make_streamlines() {
  std::vector<Streamline<float>> tracks;
  for (size_t s = 0; s != 3; ++s) {
    Streamline<float> tck;
    for (size_t v = 0; v != 4 + s; ++v)
      tck.push_back(Eigen::Vector3f(static_cast<float>(s), static_cast<float>(v), static_cast<float>(s + v)));
    tracks.push_back(tck);
  }
  return tracks;
}

//! a unique scratch path with the requested extension, cleaned up on teardown
class TractogramTest : public ::testing::Test {
protected:
  std::filesystem::path tck_path;

  void SetUp() override {
    tck_path = std::filesystem::temp_directory_path() /
               ("mrtrix_tractogram_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + ".tck");
    std::filesystem::remove(tck_path);
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(tck_path, ec);
  }
};

// Handler selection by extension (§2.6 / Step 1).
TEST_F(TractogramTest, HandlerSelectionByExtension) {
  const Formats::Base *handler = Formats::get_handler("dataset.tck");
  ASSERT_NE(handler, nullptr);
  EXPECT_EQ(handler->description, "MRtrix tracks");
  EXPECT_TRUE(handler->handles("dataset.tck"));
  EXPECT_FALSE(handler->handles("dataset.trk"));
}

// Unknown extension yields no handler (the framework raises a clean error).
TEST_F(TractogramTest, HandlerSelectionUnknownExtension) {
  EXPECT_EQ(Formats::get_handler("dataset.unknown"), nullptr);
  EXPECT_EQ(Formats::get_handler("dataset"), nullptr);
}

// Capability-flag query (§2.6): .tck is read+write, streaming, rewrite.
TEST_F(TractogramTest, CapabilityFlagQuery) {
  Formats::TCK handler;
  EXPECT_TRUE(handler.can_read());
  EXPECT_TRUE(handler.can_write());
  EXPECT_EQ(handler.capabilities.io, Formats::IO::ReadWrite);
  EXPECT_EQ(handler.capabilities.access, Formats::Access::Streaming);
  EXPECT_EQ(handler.capabilities.augment, Formats::Augment::Rewrite);
}

// Negative test: opening/creating an unknown extension raises a clean Exception.
TEST_F(TractogramTest, UnknownExtensionRaisesCleanError) {
  Properties properties;
  EXPECT_THROW(Tractogram<float>::create("dataset.unknown", properties), MR::Exception);
  EXPECT_THROW(Tractogram<float>::open("dataset.unknown", properties), MR::Exception);
}

// .tck write then read round-trips the streamline vertices through the framework.
TEST_F(TractogramTest, TckRoundTripViaFramework) {
  const std::vector<Streamline<float>> input = make_streamlines();

  {
    Properties properties;
    Tractogram<float> writer = Tractogram<float>::create(tck_path, properties);
    EXPECT_TRUE(writer.is_write());
    EXPECT_EQ(writer.format(), "MRtrix tracks");
    for (const auto &tck : input) {
      const TractogramItem<float> item(tck);
      writer(item);
    }
  } // writer destructor flushes and finalises the file

  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(tck_path, properties);
  EXPECT_TRUE(reader.is_read());

  std::vector<Streamline<float>> output;
  TractogramItem<float> item;
  while (reader(item))
    output.push_back(item.streamline);

  ASSERT_EQ(output.size(), input.size());
  for (size_t s = 0; s != input.size(); ++s) {
    ASSERT_EQ(output[s].size(), input[s].size());
    for (size_t v = 0; v != input[s].size(); ++v)
      EXPECT_TRUE(output[s][v].isApprox(input[s][v]));
  }
}

// The Stage-1 field registry is present but empty for .tck (no sidecar fields).
TEST_F(TractogramTest, FieldRegistryEmptyForTck) {
  Properties properties;
  Tractogram<float> writer = Tractogram<float>::create(tck_path, properties);
  EXPECT_TRUE(writer.fields().empty());
  EXPECT_EQ(writer.fields().size(), 0u);
}

} // namespace
