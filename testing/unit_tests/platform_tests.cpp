/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "platform.h"
#include <gtest/gtest.h>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/unistd.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#include <algorithm>
#endif

#include <filesystem>

using namespace MR::Platform;

TEST(ExecutablePathTest, ReturnsExistingAbsolutePath) {
  std::filesystem::path p;
  EXPECT_NO_THROW(p = get_executable_path());
  EXPECT_FALSE(p.empty()) << "Returned path should not be empty";
  EXPECT_TRUE(p.is_absolute()) << "Returned path should be absolute: " << p;
  EXPECT_TRUE(std::filesystem::exists(p)) << "Path must exist: " << p;
  EXPECT_TRUE(std::filesystem::is_regular_file(p)) << "Path should be a regular file: " << p;

  // canonicalize should succeed for a valid on-disk executable.
  EXPECT_NO_THROW({
    auto c = std::filesystem::canonical(p);
    (void)c;
  });
}

TEST(ExecutablePathTest, PlatformExecutableCheck) {
  const auto p = get_executable_path();

#if defined(_WIN32)
  ASSERT_TRUE(p.has_extension()) << "Executable should have an extension on Windows: " << p;
  std::string ext = p.extension().string();
  std::transform(
      ext.begin(), ext.end(), ext.begin(), [](auto ch) -> char { return static_cast<char>(std::tolower(ch)); });
  EXPECT_EQ(ext, ".exe") << "Expected .exe extension for test binary on Windows: " << p;
#elif defined(__linux__) || defined(__APPLE__)
  // On POSIX, check that the file is marked executable for current user.
  const char *cpath = p.c_str();
  const int ok = access(cpath, X_OK);
  EXPECT_EQ(ok, 0) << "Executable should be executable for current user: " << p;
#else
  static_assert(false, "Unsupported platform");
#endif
}
