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
#include "dwi/tractography/formats/pipe.h"
#include "dwi/tractography/formats/tck.h"
#include "dwi/tractography/formats/trk.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"
#include "raw.h"

#include <gtest/gtest.h>

#include <Eigen/Core>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
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
  std::filesystem::path trk_path;

  void SetUp() override {
    const std::string stem =
        "mrtrix_tractogram_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed());
    tck_path = std::filesystem::temp_directory_path() / (stem + ".tck");
    trk_path = std::filesystem::temp_directory_path() / (stem + ".trk");
    std::filesystem::remove(tck_path);
    std::filesystem::remove(trk_path);
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(tck_path, ec);
    std::filesystem::remove(trk_path, ec);
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

// The inter-command pipe handler is selected on the "-" dash token (Step 1) and
//   broadcasts streaming-only, rewrite-only read+write capabilities (Step 3).
TEST_F(TractogramTest, PipeHandlerSelectionAndCapabilities) {
  const Formats::Base *handler = Formats::get_handler("-");
  ASSERT_NE(handler, nullptr);
  EXPECT_EQ(handler->description, "piped tracks");
  EXPECT_TRUE(handler->handles("-"));
  EXPECT_FALSE(handler->handles("dataset.tck"));

  Formats::Pipe pipe_handler;
  EXPECT_TRUE(pipe_handler.can_read());
  EXPECT_TRUE(pipe_handler.can_write());
  EXPECT_EQ(pipe_handler.capabilities.io, Formats::IO::ReadWrite);
  EXPECT_EQ(pipe_handler.capabilities.access, Formats::Access::Streaming);
  EXPECT_EQ(pipe_handler.capabilities.augment, Formats::Augment::Rewrite);
}

// A streaming-only handler must reject a random-access request through the
//   framework with a clean Exception (Step 3). The pipe is the canonical
//   streaming-only format; the rejection is exercised here on a .tck Tractogram,
//   which shares the identical Access::Streaming model and therefore the same
//   framework code path (a unit test cannot stand up a live stdin/stdout pipe).
TEST_F(TractogramTest, StreamingHandlerRejectsRandomAccess) {
  Properties properties;
  Tractogram<float> writer = Tractogram<float>::create(tck_path, properties);
  EXPECT_FALSE(writer.is_random_access());
  EXPECT_THROW(writer.require_random_access("indexed streamline retrieval"), MR::Exception);
}

// The TrackVis ".trk" handler is selected by extension and broadcasts
//   streaming-only, rewrite-only read+write capabilities (Stage 14, steps 3/6).
TEST_F(TractogramTest, TrkHandlerSelectionAndCapabilities) {
  const Formats::Base *handler = Formats::get_handler("dataset.trk");
  ASSERT_NE(handler, nullptr);
  EXPECT_EQ(handler->description, "TrackVis TRK");
  EXPECT_TRUE(handler->handles("dataset.trk"));
  EXPECT_FALSE(handler->handles("dataset.tck"));

  Formats::TRK trk_handler;
  EXPECT_TRUE(trk_handler.can_read());
  EXPECT_TRUE(trk_handler.can_write());
  EXPECT_EQ(trk_handler.capabilities.io, Formats::IO::ReadWrite);
  EXPECT_EQ(trk_handler.capabilities.access, Formats::Access::Streaming);
  EXPECT_EQ(trk_handler.capabilities.augment, Formats::Augment::Rewrite);
}

// The ".trk" handler is streaming-only, so the framework must reject a
//   random-access request against it with a clean Exception (Stage 14, step 6).
TEST_F(TractogramTest, TrkHandlerRejectsRandomAccess) {
  Properties properties;
  Tractogram<float> writer = Tractogram<float>::create(trk_path, properties);
  EXPECT_FALSE(writer.is_random_access());
  EXPECT_THROW(writer.require_random_access("indexed streamline retrieval"), MR::Exception);
}

// ".trk" write then read round-trips the streamline vertices through the
//   framework, exercising the voxel-millimetre <-> scanner-space coordinate
//   conversion against the self-consistent default grid (no reference Header):
//   with an identity vox_to_ras and 1 mm isotropic spacing the conversion is the
//   identity, so the vertices must survive exactly (Stage 14, steps 2/3/4).
TEST_F(TractogramTest, TrkRoundTripCoordinatesViaFramework) {
  const std::vector<Streamline<float>> input = make_streamlines();

  {
    Properties properties;
    Tractogram<float> writer = Tractogram<float>::create(trk_path, properties);
    EXPECT_TRUE(writer.is_write());
    EXPECT_EQ(writer.format(), "TrackVis TRK");
    for (const auto &tck : input) {
      const TractogramItem<float> item(tck);
      writer(item);
    }
  } // writer destructor flushes and finalises the file

  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(trk_path, properties);
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

// A byte-swapped (opposite-endian) ".trk" must be detected via the hdr_size
//   sentinel and read back with the identical coordinates (Stage 14, step 2).
//   A little-endian ".trk" is written through the framework, then every
//   multi-byte header and body field is byte-reversed to synthesise a big-endian
//   file; opening that through the framework must reproduce the input exactly.
TEST_F(TractogramTest, TrkByteSwapDetection) {
  const std::vector<Streamline<float>> input = make_streamlines();
  {
    Properties properties;
    Tractogram<float> writer = Tractogram<float>::create(trk_path, properties);
    for (const auto &tck : input)
      writer(TractogramItem<float>(tck));
  }

  // Slurp the little-endian file and byte-reverse it into the big-endian variant.
  std::vector<std::byte> bytes;
  {
    std::ifstream in(trk_path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(in.good());
    const std::streamsize size = in.tellg();
    in.seekg(0);
    bytes.resize(static_cast<size_t>(size));
    in.read(reinterpret_cast<char *>(bytes.data()), size);
  }
  ASSERT_GE(bytes.size(), Formats::TRKUtils::header_bytes);

  Formats::TRKUtils::Header header;
  std::memcpy(&header, bytes.data(), Formats::TRKUtils::header_bytes);
  // Swap the multi-byte header fields used by the reader (incl. hdr_size, which
  //   the reader's sentinel check relies on; here n_scalars/n_properties are 0).
  for (size_t axis = 0; axis != 3; ++axis) {
    header.dim[axis] = MR::ByteOrder::swap(header.dim[axis]);
    header.voxel_size[axis] = MR::ByteOrder::swap(header.voxel_size[axis]);
  }
  for (size_t row = 0; row != 4; ++row)
    for (size_t col = 0; col != 4; ++col)
      header.vox_to_ras[row][col] = MR::ByteOrder::swap(header.vox_to_ras[row][col]);
  header.n_count = MR::ByteOrder::swap(header.n_count);
  header.version = MR::ByteOrder::swap(header.version);
  header.hdr_size = MR::ByteOrder::swap(header.hdr_size);
  std::memcpy(bytes.data(), &header, Formats::TRKUtils::header_bytes);

  // Swap the body: each record is int32 n_points + n_points*3 float (no scalars).
  size_t offset = Formats::TRKUtils::header_bytes;
  while (offset + sizeof(int32_t) <= bytes.size()) {
    const int32_t npoints = MR::Raw::fetch_LE<int32_t>(bytes.data() + offset);
    MR::Raw::store_BE<int32_t>(npoints, bytes.data() + offset);
    offset += sizeof(int32_t);
    for (int32_t i = 0; i != npoints * 3; ++i) {
      const float value = MR::Raw::fetch_LE<float>(bytes.data() + offset);
      MR::Raw::store_BE<float>(value, bytes.data() + offset);
      offset += sizeof(float);
    }
  }

  {
    std::ofstream out(trk_path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }

  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(trk_path, properties);
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
