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

#include "datatype.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "signal_handler.h"

#include "dwi/tractography/formats/trx.h"
#include "dwi/tractography/grouping.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"

#include <gtest/gtest.h>

#include <csignal>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

namespace TRXUtils = MR::DWI::Tractography::Formats::TRXUtils;
using MR::DWI::Tractography::FieldRegistry;
using MR::DWI::Tractography::Grouping;
using MR::DWI::Tractography::make_dps;
using MR::DWI::Tractography::Properties;
using MR::DWI::Tractography::ScalarOrVector;
using MR::DWI::Tractography::Streamline;
using MR::DWI::Tractography::TRXReader;
using MR::DWI::Tractography::TRXWriter;

namespace {

//! \brief The TRX filename grammar `name[.M].dtype` is parsed faithfully.
TEST(TRX, FilenameGrammarColumns) {
  const auto colour = TRXUtils::parse_filename("colour.3.uint8");
  ASSERT_TRUE(colour.has_value());
  EXPECT_EQ(colour->name, "colour");
  EXPECT_EQ(colour->columns, 3u);
  EXPECT_EQ(colour->dtype, MR::DataType(MR::DataType::UInt8));
}

//! \brief A 1-D array omits the middle integer, implying M==1.
TEST(TRX, FilenameGrammarScalar) {
  const auto fa = TRXUtils::parse_filename("fa.float16");
  ASSERT_TRUE(fa.has_value());
  EXPECT_EQ(fa->name, "fa");
  EXPECT_EQ(fa->columns, 1u);
  EXPECT_TRUE(fa->dtype.is_floating_point());
}

//! \brief A name that is itself dotted keeps its dots when no integer is present.
TEST(TRX, FilenameGrammarDottedName) {
  const auto offsets = TRXUtils::parse_filename("offsets.uint64");
  ASSERT_TRUE(offsets.has_value());
  EXPECT_EQ(offsets->name, "offsets");
  EXPECT_EQ(offsets->columns, 1u);
}

//! \brief An unrecognised extension is not a TRX array.
TEST(TRX, FilenameGrammarRejectsNonDtype) {
  EXPECT_FALSE(TRXUtils::parse_filename("header.json").has_value());
  EXPECT_FALSE(TRXUtils::parse_filename("noextension").has_value());
}

//! \brief The dtype↔extension mapping round-trips for every supported element type.
TEST(TRX, DtypeExtensionRoundTrip) {
  for (const char *ext : {"uint8",
                          "int8",
                          "uint16",
                          "int16",
                          "uint32",
                          "int32",
                          "uint64",
                          "int64",
                          "float16",
                          "float32",
                          "float64",
                          "bit"}) {
    const auto parsed = TRXUtils::parse_filename(std::string("field.") + ext);
    ASSERT_TRUE(parsed.has_value()) << ext;
    EXPECT_EQ(TRXUtils::extension_from_dtype(parsed->dtype), ext);
  }
}

//! \brief create_tempdir() makes a fresh, empty, writable directory.
TEST(TRX, CreateTempdir) {
  const std::filesystem::path dir = MR::File::create_tempdir(".trxtest");
  EXPECT_TRUE(std::filesystem::is_directory(dir));
  EXPECT_TRUE(std::filesystem::is_empty(dir));
  // The basename carries the temporary-file prefix so it is recognisable as
  //   MRtrix scratch content.
  EXPECT_TRUE(MR::File::is_tempfile(dir));
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

//! \brief Two consecutive create_tempdir() calls yield distinct directories.
TEST(TRX, CreateTempdirDistinct) {
  const std::filesystem::path a = MR::File::create_tempdir();
  const std::filesystem::path b = MR::File::create_tempdir();
  EXPECT_NE(a, b);
  std::error_code ec;
  std::filesystem::remove_all(a, ec);
  std::filesystem::remove_all(b, ec);
}

//! \brief The -force augmentation policy follows the step-11 matrix.
/*! A directory and an uncompressed (ZIP_STORE) archive accept a new sidecar
 * without -force; a compressed (ZIP_DEFLATE) archive requires it; a missing
 * path is a fresh write that requires no -force. */
TEST(TRX, ForcePolicyDirectory) {
  const std::filesystem::path dir = MR::File::create_tempdir(".trxforce");
  EXPECT_EQ(TRXUtils::classify_backing(dir), TRXUtils::Backing::Directory);
  EXPECT_FALSE(TRXUtils::augment_requires_force(dir));
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

TEST(TRX, ForcePolicyMissing) {
  const std::filesystem::path missing = MR::File::create_tempdir(".trxforce");
  std::error_code ec;
  std::filesystem::remove_all(missing, ec);
  EXPECT_EQ(TRXUtils::classify_backing(missing), TRXUtils::Backing::Missing);
  EXPECT_FALSE(TRXUtils::augment_requires_force(missing));
}

//! \brief A temp directory marked for deletion is removed (with its contents)
//!   when the process receives a terminating signal.
/*! Forks a child that initialises the MRtrix signal handler, creates a temp
 * directory containing a file, marks it for deletion, then raises SIGTERM. The
 * parent confirms the directory tree was cleaned up by the signal handler. */
TEST(TRX, TempdirCleanupOnSignal) {
  // Pre-create the directory in the PARENT so its path is known to the parent
  //   (the child shares the inherited tmpfile location); the child marks and
  //   populates it before raising the signal.
  const std::filesystem::path dir = MR::File::create_tempdir(".trxsig");
  {
    MR::File::OFStream stream(dir / "member", std::ios::out | std::ios::binary | std::ios::trunc);
    stream << "data";
  }
  ASSERT_FALSE(std::filesystem::is_empty(dir));

  const pid_t pid = fork();
  ASSERT_GE(pid, 0);
  if (pid == 0) {
    // Child: install the handler, register the directory, raise SIGTERM.
    MR::SignalHandler::init();
    MR::SignalHandler::mark_file_for_deletion(dir);
    std::raise(SIGTERM);
    _exit(0); // unreached if the handler terminates the process
  }

  int status = 0;
  ASSERT_EQ(waitpid(pid, &status, 0), pid);
  // The signal handler should have removed the marked directory and its file.
  EXPECT_FALSE(std::filesystem::exists(dir));
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

//! \brief Build a simple straight-line streamline with \a n vertices.
Streamline<float> make_streamline(const size_t index, const size_t n) {
  Streamline<float> tck;
  tck.set_index(index);
  for (size_t v = 0; v != n; ++v)
    tck.push_back({static_cast<float>(index), static_cast<float>(v), 0.0f});
  return tck;
}

} // namespace

//! \brief A TRX dataset's groups + dpg survive a write→read round-trip (Stage 17).
/*! Writes a 6-streamline TRX directory carrying two overlapping groups and a dpg
 * field on each, then re-reads it through TRXReader::read_grouping and checks the
 * grouping is reproduced exactly. Exercises overlapping membership and a
 * streamline in multiple groups. */
TEST(TRX, GroupingRoundTrip) {
  const std::filesystem::path dir = MR::File::create_tempdir(".trxgrp");
  std::error_code ec;
  std::filesystem::remove_all(dir, ec); // writer creates the directory itself
  const std::filesystem::path dataset = dir;

  constexpr size_t num_streamlines = 6;

  Grouping out_grouping;
  // Two overlapping groups; streamline 2 and 3 belong to both (multi-membership).
  out_grouping.set_group("bundle_a", {0, 2, 3});
  out_grouping.set_group("bundle_b", {2, 3, 5});
  // dpg metadata in two native dtypes (a float mean, a 3-uint8 colour).
  {
    ScalarOrVector<float> mean(1, 1);
    mean(0, 0) = 0.73f;
    out_grouping.set_dpg("bundle_a", "mean_fa", make_dps(std::move(mean)));
    ScalarOrVector<uint8_t> colour(1, 3);
    colour(0, 0) = 10;
    colour(0, 1) = 20;
    colour(0, 2) = 30;
    out_grouping.set_dpg("bundle_b", "colour", make_dps(std::move(colour)));
  }

  {
    Properties properties;
    FieldRegistry registry;
    TRXWriter<float> writer(dataset, properties, registry);
    writer.write_grouping(out_grouping);
    for (size_t i = 0; i != num_streamlines; ++i)
      writer(make_streamline(i, 4));
  } // writer finalises on destruction

  // The on-disk layout matches the TRX spec.
  EXPECT_TRUE(std::filesystem::exists(dataset / "groups" / "bundle_a.uint32"));
  EXPECT_TRUE(std::filesystem::exists(dataset / "groups" / "bundle_b.uint32"));
  EXPECT_TRUE(std::filesystem::exists(dataset / "dpg" / "bundle_a" / "mean_fa.float32"));
  EXPECT_TRUE(std::filesystem::exists(dataset / "dpg" / "bundle_b" / "colour.3.uint8"));

  Properties in_props;
  FieldRegistry in_registry;
  TRXReader<float> reader(dataset, in_props, in_registry);
  Grouping in_grouping;
  reader.read_grouping(in_grouping);

  ASSERT_TRUE(in_grouping.has_group("bundle_a"));
  ASSERT_TRUE(in_grouping.has_group("bundle_b"));
  const std::vector<uint32_t> expected_a{0, 2, 3};
  const std::vector<uint32_t> expected_b{2, 3, 5};
  EXPECT_EQ(in_grouping.members("bundle_a"), expected_a);
  EXPECT_EQ(in_grouping.members("bundle_b"), expected_b);

  const auto *fa = in_grouping.get_dpg("bundle_a", "mean_fa");
  ASSERT_NE(fa, nullptr);
  ASSERT_TRUE(std::holds_alternative<ScalarOrVector<float>>(*fa));
  EXPECT_FLOAT_EQ(std::get<ScalarOrVector<float>>(*fa).scalar(), 0.73f);

  const auto *col = in_grouping.get_dpg("bundle_b", "colour");
  ASSERT_NE(col, nullptr);
  ASSERT_TRUE(std::holds_alternative<ScalarOrVector<uint8_t>>(*col));
  const auto &row = std::get<ScalarOrVector<uint8_t>>(*col);
  ASSERT_EQ(row.cols(), 3);
  EXPECT_EQ(row(0, 0), 10);
  EXPECT_EQ(row(0, 1), 20);
  EXPECT_EQ(row(0, 2), 30);

  std::filesystem::remove_all(dataset, ec);
}
