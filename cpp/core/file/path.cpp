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

#include "file/path.h"

namespace MR::Path {

const std::string home_env("HOME");

std::string basename(std::string_view name) {
  size_t i = name.find_last_of(PATH_SEPARATORS);
  return (i == std::string::npos ? std::string(name) : std::string(name.substr(i + 1)));
}

std::string dirname(std::string_view name) {
  size_t i = name.find_last_of(PATH_SEPARATORS);
  return (i == std::string::npos ? std::string("")
                                 : (i ? std::string(name.substr(0, i)) : std::string(1, PATH_SEPARATORS[0])));
}

std::string join(std::string_view first, std::string_view second) {
  if (first.empty())
    return std::string(second);
  if (first[first.size() - 1] != PATH_SEPARATORS[0]
#ifdef MRTRIX_WINDOWS
      && first[first.size() - 1] != PATH_SEPARATORS[1]
#endif
  )
    return std::string(first) + PATH_SEPARATORS[0] + second;
  return std::string(first) + second;
}

bool exists(std::string_view path) {
  struct stat buf;
#ifdef MRTRIX_WINDOWS
  const std::string stripped(strip(path, PATH_SEPARATORS, false, true));
  if (!stat(stripped.c_str(), &buf))
#else
  if (!stat(std::string(path).c_str(), &buf))
#endif
    return true;
  if (errno == ENOENT)
    return false;
  throw Exception(strerror(errno));
  return false;
}

bool is_dir(std::string_view path) {
  struct stat buf;
#ifdef MRTRIX_WINDOWS
  const std::string stripped(strip(path, PATH_SEPARATORS, false, true));
  if (!stat(stripped.c_str(), &buf))
#else
  if (stat(std::string(path).c_str(), &buf) == 0)
#endif
    return S_ISDIR(buf.st_mode);
  if (errno == ENOENT)
    return false;
  throw Exception(strerror(errno));
  return false;
}

bool is_file(std::string_view path) {
  struct stat buf;
  if (stat(std::string(path).c_str(), &buf) == 0)
    return S_ISREG(buf.st_mode);
  if (errno == ENOENT)
    return false;
  throw Exception(strerror(errno));
  return false;
}

bool has_suffix(std::string_view name, std::string_view suffix) {
  return name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool has_suffix(std::string_view name, const std::initializer_list<const std::string> &suffix_list) {
  return std::any_of(
      suffix_list.begin(), suffix_list.end(), [&](std::string_view suffix) { return has_suffix(name, suffix); });
}

bool has_suffix(std::string_view name, const std::vector<std::string> &suffix_list) {
  return std::any_of(
      suffix_list.begin(), suffix_list.end(), [&](std::string_view suffix) { return has_suffix(name, suffix); });
}

bool is_mrtrix_image(std::string_view name) {
  return name == "-" || Path::has_suffix(name, {".mif", ".mih", ".mif.gz"});
}

char delimiter(std::string_view filename) {
  if (Path::has_suffix(filename, ".tsv"))
    return '\t';
  return Path::has_suffix(filename, ".csv") ? ',' : ' ';
}

std::string cwd() {
  std::string path;
  size_t buf_size = 32;
  while (true) {
    path.reserve(buf_size);
    if (getcwd(&path[0], buf_size) != nullptr)
      break;
    if (errno != ERANGE)
      throw Exception("failed to get current working directory!");
    buf_size *= 2;
  }
  return path;
}

std::string home() {
  const char *home = getenv(home_env.c_str()); // check_syntax off
  if (home == nullptr)
    throw Exception(home_env + " environment variable is not set!");
  return home;
}

Dir::Dir(std::string_view name) : p(opendir(!name.empty() ? std::string(name).c_str() : ".")) {
  if (p == nullptr)
    throw Exception("error opening folder " + name + ": " + strerror(errno));
}
Dir::~Dir() {
  if (p != nullptr)
    closedir(p);
}

std::string Dir::read_name() {
  std::string ret;
  struct dirent *entry = readdir(p);
  if (entry != nullptr) {
    ret = std::string(entry->d_name);
    if (ret == "." || ret == "..")
      ret = read_name();
  }
  return ret;
}

void Dir::close() {
  if (p != nullptr)
    closedir(p);
  p = nullptr;
}

} // namespace MR::Path
