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

#include "app.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/formats/pipe.h"
#include "dwi/tractography/formats/ram.h"
#include "dwi/tractography/formats/tck.h"
#include "dwi/tractography/formats/trk.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar_value.h"
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
#include <optional>
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
  std::filesystem::path vtk_path;

  void SetUp() override {
    const std::string stem =
        "mrtrix_tractogram_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed());
    tck_path = std::filesystem::temp_directory_path() / (stem + ".tck");
    trk_path = std::filesystem::temp_directory_path() / (stem + ".trk");
    vtk_path = std::filesystem::temp_directory_path() / (stem + ".vtk");
    std::filesystem::remove(tck_path);
    std::filesystem::remove(trk_path);
    std::filesystem::remove(vtk_path);
    // No App is stood up in a unit-test context, so the global overwrite flag
    //   defaults to false; the ".vtk" writer's App::check_overwrite() guard would
    //   then reject a scratch path written more than once within a test. These
    //   tests own their unique scratch paths, so permit overwriting (== a command
    //   run with -force, or a fresh output).
    MR::App::overwrite_files = true;
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(tck_path, ec);
    std::filesystem::remove(trk_path, ec);
    std::filesystem::remove(vtk_path, ec);
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

// Stage 4: TCK::binary_layout() locates the binary vertex block from the header
//   alone (data file, byte offset, on-disk datatype) for the mrview raw-block
//   fast path. The offset it reports must seek exactly to the first vertex, and
//   the datatype must match the (native) write datatype Float32.
TEST_F(TractogramTest, TckBinaryLayoutLocatesVertexBlock) {
  const std::vector<Streamline<float>> input = make_streamlines();
  {
    Properties properties;
    Tractogram<float> writer = Tractogram<float>::create(tck_path, properties);
    for (const auto &tck : input)
      writer(TractogramItem<float>(tck));
  }

  Formats::TCK handler;
  const std::optional<Formats::TCKBinaryLayout> layout = handler.binary_layout(tck_path);
  ASSERT_TRUE(layout.has_value());
  EXPECT_EQ(layout->data_path, tck_path);
  EXPECT_EQ(layout->datatype() & MR::DataType::Type, MR::DataType::Float32);
  EXPECT_GT(layout->data_offset, 0);

  // The reported offset must seek exactly to the first streamline's first
  //   vertex: read three floats there and compare to the synthetic input.
  std::ifstream in(tck_path, std::ios::binary);
  ASSERT_TRUE(in.good());
  in.seekg(layout->data_offset);
  std::array<float, 3> first{};
  in.read(reinterpret_cast<char *>(first.data()), sizeof(first));
  ASSERT_TRUE(in.good());
  EXPECT_FLOAT_EQ(first[0], input[0][0][0]);
  EXPECT_FLOAT_EQ(first[1], input[0][0][1]);
  EXPECT_FLOAT_EQ(first[2], input[0][0][2]);
}

// Stage 4: binary_layout() returns nullopt (clean fall-back signal, not an
//   error) for a file lacking the header information needed to locate the block.
TEST_F(TractogramTest, TckBinaryLayoutNulloptForMalformedHeader) {
  {
    std::ofstream out(tck_path, std::ios::binary | std::ios::trunc);
    out << "mrtrix tracks\nEND\n";
  }
  Formats::TCK handler;
  EXPECT_FALSE(handler.binary_layout(tck_path).has_value());
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

// =====================================================================
// Stage 15 — random-access RAM wrapper around a streaming-only format
// =====================================================================

// Helper: write the synthetic streamlines to a streaming ".vtk" via the framework.
void write_streaming_vtk(const std::filesystem::path &path, const std::vector<Streamline<float>> &input) {
  Properties properties;
  Tractogram<float> writer = Tractogram<float>::create(path, properties);
  EXPECT_FALSE(writer.is_random_access());
  for (const auto &tck : input)
    writer(TractogramItem<float>(tck));
}

// Step 2: requesting random access against a streaming-only format (".vtk")
//   transparently selects the in-RAM wrapper rather than raising the
//   streaming-only error. The wrapper advertises full random access and exposes
//   the RAM store.
TEST_F(TractogramTest, RamWrapperSelectedForStreamingRandomAccess) {
  const std::vector<Streamline<float>> input = make_streamlines();
  write_streaming_vtk(vtk_path, input);

  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(vtk_path, properties, AccessRequest::RandomAccess);
  EXPECT_TRUE(reader.is_read());
  EXPECT_TRUE(reader.is_random_access());
  EXPECT_TRUE(reader.has_ram_store());
  EXPECT_EQ(reader.capabilities().access, Formats::Access::RandomAccessFull);
  // The whole dataset is resident: the indexed count is available immediately.
  ASSERT_EQ(reader.size(), input.size());
}

// Step 1: the wrapper loads the WHOLE dataset into RAM on construction, so any
//   streamline is addressable at any time, in any order (out-of-order indexed
//   retrieval against the resident store).
TEST_F(TractogramTest, RamWrapperRandomAccessOutOfOrderVtk) {
  const std::vector<Streamline<float>> input = make_streamlines();
  write_streaming_vtk(vtk_path, input);

  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(vtk_path, properties, AccessRequest::RandomAccess);
  ASSERT_EQ(reader.size(), input.size());

  // Read streamline N out of order (last, first, middle) and confirm the vertices.
  const std::vector<size_t> order = {input.size() - 1, 0, input.size() / 2};
  for (const size_t s : order) {
    const auto &item = reader[s];
    ASSERT_EQ(item.streamline.size(), input[s].size());
    for (size_t v = 0; v != input[s].size(); ++v)
      EXPECT_TRUE(item.streamline[v].isApprox(input[s][v]));
  }
}

// Step 1: load-once / write-once. The inner streaming handler touches the
//   filesystem only at construction (load) and destruction (flush). A wrapped
//   write that mutates the RAM store (append / erase / set) commits to disk
//   exactly once, on destruction, and reloading reproduces the mutated dataset.
TEST_F(TractogramTest, RamWrapperWriteOnceFlushVtk) {
  const std::vector<Streamline<float>> input = make_streamlines();

  {
    Properties properties;
    Tractogram<float> writer =
        Tractogram<float>::create(vtk_path, properties, FieldRegistry(), AccessRequest::RandomAccess);
    EXPECT_TRUE(writer.is_random_access());
    EXPECT_TRUE(writer.has_ram_store());
    // No file is written yet: everything is buffered in RAM (write-once on dtor).
    EXPECT_FALSE(std::filesystem::exists(vtk_path));
    for (const auto &tck : input)
      writer.append(TractogramItem<float>(tck));
    EXPECT_EQ(writer.size(), input.size());
    // Mutate the resident store: drop the middle streamline (RandomAccessFull).
    writer.erase(1);
    EXPECT_EQ(writer.size(), input.size() - 1);
  } // destructor flushes the RAM store to disk exactly once via the inner handler

  ASSERT_TRUE(std::filesystem::exists(vtk_path));

  // Reload (streaming) and confirm the file holds the mutated set (streamlines 0 and 2).
  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(vtk_path, properties);
  std::vector<Streamline<float>> output;
  TractogramItem<float> item;
  while (reader(item))
    output.push_back(item.streamline);

  ASSERT_EQ(output.size(), input.size() - 1);
  const std::vector<size_t> expected = {0, 2};
  for (size_t i = 0; i != expected.size(); ++i) {
    const size_t s = expected[i];
    ASSERT_EQ(output[i].size(), input[s].size());
    for (size_t v = 0; v != input[s].size(); ++v)
      EXPECT_TRUE(output[i][v].isApprox(input[s][v]));
  }
}

// Step 1: the sidecar (dps/dpv, native dtype, M>1) is carried through the RAM
//   store intact. A ".vtk" with a per-streamline (dps, int32, M=1 scalar) field
//   and a per-vertex (dpv, uint8, M=3 RGB matrix) field is written through the
//   RAM wrapper, then reopened for random access; the resident items must
//   reproduce both fields exactly, with their native variant alternative.
TEST_F(TractogramTest, RamWrapperSidecarCarriedThroughVtk) {
  const std::vector<Streamline<float>> tracks = make_streamlines();

  // Declare the output field set: one dps (int32 scalar) + one dpv (uint8 RGB).
  FieldRegistry registry;
  const size_t dps_ord =
      registry.add({"bundle", FieldRole::DPS, MR::DataType(MR::DataType::Int32), 1, FieldSource::Internal, 0});
  const size_t dpv_ord =
      registry.add({"colour", FieldRole::DPV, MR::DataType(MR::DataType::UInt8), 3, FieldSource::Internal, 0});

  // Build the per-streamline payloads.
  auto make_item = [&](size_t s) {
    TractogramItem<float> item(tracks[s]);
    item.dps.resize(registry.dps_count());
    item.dpv.resize(registry.dpv_count());
    ScalarOrVector<int32_t> bundle(1);
    bundle(0, 0) = static_cast<int32_t>(100 + s);
    item.dps[dps_ord] = make_dps(std::move(bundle));
    VectorOrMatrix<uint8_t> colour(static_cast<Eigen::Index>(tracks[s].size()), 3);
    for (Eigen::Index v = 0; v != colour.rows(); ++v) {
      colour(v, 0) = static_cast<uint8_t>(s);
      colour(v, 1) = static_cast<uint8_t>(v);
      colour(v, 2) = static_cast<uint8_t>(s + v);
    }
    item.dpv[dpv_ord] = make_dpv(std::move(colour));
    return item;
  };

  {
    Properties properties;
    Tractogram<float> writer = Tractogram<float>::create(vtk_path, properties, registry, AccessRequest::RandomAccess);
    EXPECT_TRUE(writer.has_ram_store());
    for (size_t s = 0; s != tracks.size(); ++s)
      writer.append(make_item(s));
  } // write-once flush through the inner VTK handler on destruction

  // Reopen for random access; the wrapper re-loads everything (incl. sidecar) into RAM.
  Properties properties;
  Tractogram<float> reader = Tractogram<float>::open(vtk_path, properties, AccessRequest::RandomAccess);
  ASSERT_EQ(reader.size(), tracks.size());

  const auto *dps_desc = reader.fields().find("bundle", FieldRole::DPS);
  const auto *dpv_desc = reader.fields().find("colour", FieldRole::DPV);
  ASSERT_NE(dps_desc, nullptr);
  ASSERT_NE(dpv_desc, nullptr);
  EXPECT_EQ(dpv_desc->columns, 3u);

  for (size_t s = 0; s != tracks.size(); ++s) {
    const auto &item = reader[s];
    // dps: native int32 scalar preserved (M==1, not flattened to a vector).
    ASSERT_EQ(item.dps.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<ScalarOrVector<int32_t>>(item.dps[dps_desc->ordinal]));
    EXPECT_EQ(std::get<ScalarOrVector<int32_t>>(item.dps[dps_desc->ordinal]).scalar(), static_cast<int32_t>(100 + s));
    // dpv: native uint8 n_vertices x 3 matrix preserved (M>1 carried intact).
    ASSERT_EQ(item.dpv.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<VectorOrMatrix<uint8_t>>(item.dpv[dpv_desc->ordinal]));
    const auto &colour = std::get<VectorOrMatrix<uint8_t>>(item.dpv[dpv_desc->ordinal]);
    ASSERT_EQ(static_cast<size_t>(colour.rows()), tracks[s].size());
    ASSERT_EQ(colour.cols(), 3);
    for (Eigen::Index v = 0; v != colour.rows(); ++v) {
      EXPECT_EQ(colour(v, 0), static_cast<uint8_t>(s));
      EXPECT_EQ(colour(v, 1), static_cast<uint8_t>(v));
      EXPECT_EQ(colour(v, 2), static_cast<uint8_t>(s + v));
    }
  }
}

// Step 2: the clean streaming-only error is preserved for a format the wrapper
//   genuinely cannot satisfy. The inter-command pipe is a one-pass stdin/stdout
//   stream (can_ram_wrap() == false), so a random-access request must still raise
//   rather than being silently RAM-wrapped.
TEST_F(TractogramTest, RamWrapperNotSelectedForPipe) {
  Formats::Pipe pipe_handler;
  EXPECT_FALSE(pipe_handler.can_ram_wrap());

  Properties properties;
  EXPECT_THROW(Tractogram<float>::open("-", properties, AccessRequest::RandomAccess), MR::Exception);
}

} // namespace
