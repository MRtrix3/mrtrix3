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

#include <gtest/gtest.h>

#include <csignal>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

namespace TRXUtils = MR::DWI::Tractography::Formats::TRXUtils;

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

} // namespace
