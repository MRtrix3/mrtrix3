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

#ifdef MRTRIX_WINDOWS
#include <windows.h>
#elif defined(MRTRIX_MACOSX)
#include <mach-o/dyld.h>
#elif defined(MRTRIX_FREEBSD)
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <unistd.h>
#endif

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace MR::Platform {
std::filesystem::path get_executable_path() {
#ifdef MRTRIX_WINDOWS
  std::vector<wchar_t> buffer;
  buffer.resize(MAX_PATH);
  for (;;) {
    DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) {
      throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()),
                              "GetModuleFileNameW failed");
    }
    if (static_cast<size_t>(len) < buffer.size()) {
      return std::filesystem::path(std::wstring(buffer.data(), len));
    }
    size_t new_size = buffer.size() * 2;
    if (new_size <= buffer.size())
      throw std::runtime_error("Executable path too long");
    buffer.resize(new_size);
  }
#elif defined(MRTRIX_MACOSX)
  uint32_t size = 0;
  if (_NSGetExecutablePath(nullptr, &size) != -1)
    throw std::runtime_error("Unexpected success getting buffer size");
  std::string buffer;
  buffer.resize(size);
  if (_NSGetExecutablePath(buffer.data(), &size) != 0)
    throw std::runtime_error("_NSGetExecutablePath failed to fill buffer");
  try {
    return std::filesystem::canonical(std::filesystem::path(buffer));
  } catch (const std::filesystem::filesystem_error &) {
    return std::filesystem::weakly_canonical(std::filesystem::path(buffer));
  }
  return std::filesystem::canonical(std::filesystem::path(buffer));
#elif defined(MRTRIX_LINUX)
  const std::filesystem::path link("/proc/self/exe");
  try {
    const auto p = std::filesystem::read_symlink(link);
    try {
      return std::filesystem::canonical(p);
    } catch (const std::filesystem::filesystem_error &) {
      return std::filesystem::weakly_canonical(p);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw std::system_error(std::error_code(errno, std::system_category()),
                            std::string("read_symlink(\"/proc/self/exe\") failed: ") + e.what());
  }
#elif defined(MRTRIX_FREEBSD)
  // See https://github.com/chromium/chromium/blob/db87c489ae756af10467897d653518f321db2f4d/base/base_paths_posix.cc
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
  size_t size = 0;
  if (sysctl(mib, 4, nullptr, &size, nullptr, 0) == -1)
    throw std::system_error(std::error_code(errno, std::system_category()),
                            "sysctl(KERN_PROC_PATHNAME) failed to get size");
  std::string buffer;
  buffer.resize(size);
  if (sysctl(mib, 4, buffer.data(), &size, nullptr, 0) == -1)
    throw std::system_error(std::error_code(errno, std::system_category()),
                            "sysctl(KERN_PROC_PATHNAME) failed to get path");
  // Ensure we use the null-terminated string
  try {
    return std::filesystem::canonical(std::filesystem::path(buffer.c_str()));
  } catch (const std::filesystem::filesystem_error &) {
    return std::filesystem::weakly_canonical(std::filesystem::path(buffer.c_str()));
  }
#else
  static_assert(false, "Unsupported platform");
#endif
}
} // namespace MR::Platform
