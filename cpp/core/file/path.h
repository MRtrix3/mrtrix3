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

#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "exception.h"
#include "mrtrix.h"
#include "types.h"

#define HOME_ENV "HOME"

/*! \def PATH_SEPARATORS
 *  \brief symbols used for separating directories in filesystem paths
 *
 *  The PATH_SEPARATORS macro contains all characters that may be used
 *  to delimit directory / file names in a filesystem path. On
 *  POSIX-compliant systems, this is simply the forward-slash character
 *  '/'; on Windows however, either forward-slashes or back-slashes
 *  can appear. Therefore any code that performs such direct
 *  manipulation of filesystem paths should both use this macro, and
 *  be written accounting for the possibility of this string containing
 *  either one or two characters depending on the target system. */
#ifdef MRTRIX_WINDOWS
// Preferentially use forward-slash when inserting via PATH_SEPARATORS[0]
#define PATH_SEPARATORS "/\\"
#else
#define PATH_SEPARATORS "/"
#endif

#include <filesystem>

namespace MR::Path {

inline bool has_suffix(const std::filesystem::path &name, const std::string &suffix) {
  if (suffix.find('.') == 0 && suffix.find('.', 1) == std::string::npos) {
    if (name.extension() == suffix)
      return true;
  }
  std::string name_str = name.string();
  return name_str.size() >= suffix.size() &&
         name_str.compare(name_str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline bool has_suffix(const std::filesystem::path &name, const std::initializer_list<const std::string> &suffix_list) {
  return std::any_of(
      suffix_list.begin(), suffix_list.end(), [&](const std::string &suffix) { return has_suffix(name, suffix); });
}

inline bool has_suffix(const std::filesystem::path &name, const std::vector<std::string> &suffix_list) {
  return std::any_of(
      suffix_list.begin(), suffix_list.end(), [&](const std::string &suffix) { return has_suffix(name, suffix); });
}

inline bool is_mrtrix_image(const std::string &name) {
  return strcmp(name.c_str(), std::string("-").c_str()) == 0 ||
         Path::has_suffix(std::filesystem::path(name), {".mif", ".mih", ".mif.gz"});
}

inline std::string home() {
  const char *home = getenv(HOME_ENV);
  if (!home)
    throw Exception(HOME_ENV " environment variable is not set!");
  return home;
}

inline char delimiter(const std::string &filename) {
  if (Path::has_suffix(std::filesystem::path(filename), ".tsv"))
    return '\t';
  else if (Path::has_suffix(std::filesystem::path(filename), ".csv"))
    return ',';
  else
    return ' ';
}

} // namespace MR::Path
