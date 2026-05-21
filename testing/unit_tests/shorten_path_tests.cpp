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

#include "gtest/gtest.h"

#include "mrtrix.h"

#include <filesystem>
#include <ostream>
#include <string>

using namespace MR;
namespace fs = std::filesystem;

namespace {

struct ShortenPathCase {
  fs::path path;
  size_t longest;
  std::string expected;
};

void PrintTo(const ShortenPathCase &c, std::ostream *os) {
  *os << "{path=\"" << c.path.string() << "\", longest=" << c.longest << "}";
}

} // namespace

class ShortenPathTest : public ::testing::TestWithParam<ShortenPathCase> {};

// Paths whose string representation is shorter than longest: returned unchanged.
INSTANTIATE_TEST_SUITE_P(Passthrough,
                         ShortenPathTest,
                         ::testing::Values(
                             // bare filename, well within default limit
                             ShortenPathCase{"data.txt", 40, "data.txt"},
                             // absolute path with several directories, well within limit
                             ShortenPathCase{"/usr/local/bin/file.txt", 40, "/usr/local/bin/file.txt"},
                             // relative path, well within limit
                             ShortenPathCase{"relative/path/data.txt", 40, "relative/path/data.txt"},
                             // many short single-character directory names
                             ShortenPathCase{"/a/b/c/d/e/f/g.txt", 40, "/a/b/c/d/e/f/g.txt"},
                             // custom limit, path just fits (21 < 22)
                             ShortenPathCase{"/home/user/report.txt", 22, "/home/user/report.txt"},
                             // path length one below the boundary (13 < 14)
                             ShortenPathCase{"/a/b/file.txt", 14, "/a/b/file.txt"}));

// Paths where filename.size() + 5 > longest: only filename returned, no directory context.
INSTANTIATE_TEST_SUITE_P(FilenameOnly,
                         ShortenPathTest,
                         ::testing::Values(
                             // deeply nested, filename barely fits with no room for separator sequence
                             ShortenPathCase{"/a/b/c/data.txt", 12, "data.txt"},
                             // longer path, longer filename
                             ShortenPathCase{"/very/long/path/report.txt", 14, "report.txt"},
                             // relative path
                             ShortenPathCase{"subdir/file.txt", 9, "file.txt"},
                             // single-character filename still too tight for any directory context
                             ShortenPathCase{"/a/b/c/d/e/f/ab.txt", 10, "ab.txt"}));

// Paths requiring middle elision: the algorithm alternately grows prefix (from the
// left end) and suffix (from the right end) and elides the unconsumed middle.
//
// On POSIX systems path::root_name() is always empty, so root-name presence is
// not exercisable here.  Absolute vs. relative paths cover the closest analogue:
// the root directory "/" appears in the prefix when the leftmost directory does
// not fit in the budget, giving the "/.../filename" form; relative paths elide
// to "firstdir/.../filename" without a leading separator.
INSTANTIATE_TEST_SUITE_P(Elision,
                         ShortenPathTest,
                         ::testing::Values(
                             // prefix grows to "aa", suffix to "ee/file.txt"; "bb/cc/dd" elided
                             ShortenPathCase{"/aa/bb/cc/dd/ee/file.txt", 18, "aa/.../ee/file.txt"},
                             // short filename allows only one component on each side
                             ShortenPathCase{"/aa/bb/cc/dd/ee/ff/g.txt", 15, "aa/.../ff/g.txt"},
                             // long middle directory prevents prefix from growing past one component
                             ShortenPathCase{"/a/long_middle_dir/b/file.txt", 16, "a/.../b/file.txt"},
                             // path length equals longest (< not <=): still elided even though no shorter
                             ShortenPathCase{"/a/b/file.txt", 13, "/.../file.txt"},
                             // first directory too wide for the budget: prefix stays empty, giving "/.../filename"
                             ShortenPathCase{"/verylongdir/subdir/file.txt", 20, "/.../file.txt"},
                             // relative path: no root separator; prefix grows from first component
                             ShortenPathCase{"aa/bb/cc/dd/ee/file.txt", 15, "aa/.../file.txt"}));

TEST_P(ShortenPathTest, MatchesExpected) {
  const ShortenPathCase &c = GetParam();
  EXPECT_EQ(shorten(c.path, c.longest), c.expected);
}
