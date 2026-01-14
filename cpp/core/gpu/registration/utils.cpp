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

#include "gpu/registration/utils.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#endif

using namespace std::string_literals;

uint32_t Utils::nextMultipleOf(const uint32_t value, const uint32_t multiple) {
  if (value > std::numeric_limits<uint32_t>::max() - multiple) {
    return std::numeric_limits<uint32_t>::max();
  }
  return (value + multiple - 1) / multiple * multiple;
}

std::string Utils::readFile(const std::filesystem::path &filePath, ReadFileMode mode) {
  if (!std::filesystem::exists(filePath)) {
    throw std::runtime_error("File not found: "s + filePath.string());
  }

  const auto openMode = (mode == ReadFileMode::Binary) ? std::ios::in | std::ios::binary : std::ios::in;
  std::ifstream f(filePath, std::ios::in | openMode);
  const auto fileSize64 = std::filesystem::file_size(filePath);
  if (fileSize64 > static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    throw std::runtime_error("File too large to read into memory: "s + filePath.string());
  }
  const std::streamsize fileSize = static_cast<std::streamsize>(fileSize64);
  std::string result(static_cast<std::string::size_type>(fileSize), '\0');
  f.read(result.data(), fileSize);

  return result;
}

std::filesystem::path Utils::getExecutablePath() {
#if defined(_WIN32)
  wchar_t buffer[MAX_PATH];
  const DWORD len = GetModuleFileNameW(NULL, buffer, MAX_PATH);
  if (len == 0) {
    throw std::runtime_error("GetModuleFileNameW failed. Error: " + std::to_string(GetLastError()));
  }
  if (len == MAX_PATH) {
    throw std::runtime_error("GetModuleFileNameW failed: Buffer too small, path truncated.");
  }
  return std::filesystem::path(buffer); // buffer content is copied

#elif defined(__APPLE__)
  uint32_t size = 0;
  if (_NSGetExecutablePath(nullptr, &size) != -1) {
    throw std::runtime_error("_NSGetExecutablePath: Unexpected behavior when querying buffer size.");
  }
  std::vector<char> bufferVec(size);
  if (_NSGetExecutablePath(bufferVec.data(), &size) != 0) {
    throw std::runtime_error("_NSGetExecutablePath: Failed to retrieve executable path.");
  }
  const std::filesystem::path initialPath(bufferVec.data());
  std::error_code ec;
  const std::filesystem::path canonicalPath = std::filesystem::canonical(initialPath, ec);
  if (ec) {
    throw std::runtime_error("Failed to get canonical path for '" + initialPath.string() + "': " + ec.message());
  }
  return canonicalPath;

#elif defined(__linux__)
  const std::string linkPathStr = "/proc/self/exe"; // const as it's not modified
  std::error_code ec;

  const std::filesystem::path symlinkPath(linkPathStr);
  const std::filesystem::path p = std::filesystem::read_symlink(symlinkPath, ec);

  if (ec) {
    throw std::runtime_error("read_symlink(\"" + linkPathStr + "\") failed: " + ec.message());
  }
  const std::filesystem::path canonicalP = std::filesystem::canonical(p, ec);
  if (ec) {
    throw std::runtime_error("canonical(\"" + p.string() + "\") failed: " + ec.message());
  }
  return canonicalP;

#else
#error Unsupported platform
#endif
}

std::string Utils::randomString(size_t length) {
  static const std::string characterSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  // 1. A static random engine that is seeded only once.
  static std::mt19937 generator = []() {
    std::random_device rd;
    return std::mt19937(rd());
  }();

  // 2. A distribution that maps to the indices of the character set.
  //    We create it once and reuse it.
  static std::uniform_int_distribution<std::string::size_type> distribution(0, characterSet.size() - 1);

  std::string result;
  result.reserve(length);

  // 3. Fill the string with random characters.
  for (size_t i = 0; i < length; ++i) {
    result += characterSet[distribution(generator)];
  }

  return result;
}

std::string Utils::hash_string(const std::string &input) {
  const std::hash<std::string> hasher;
  const size_t hashValue = hasher(input);
  return std::to_string(hashValue);
}
