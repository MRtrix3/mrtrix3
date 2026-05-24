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

#include "file/path.h"

#include <algorithm>
#include <optional>

#include "env.h"
#include "exception.h"

namespace MR::Path {

const std::string home_env("HOME");

bool has_suffix(const std::filesystem::path &name, std::string_view suffix) {
  if (suffix.find('.') == 0 && suffix.find('.', 1) == std::string::npos) {
    if (name.extension() == suffix)
      return true;
  }
  std::string const name_str = name.string();
  return name_str.size() >= suffix.size() &&
         name_str.compare(name_str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool has_suffix(const std::filesystem::path &name, const std::initializer_list<const std::string> &suffix_list) {
  return std::any_of(suffix_list.begin(),                                                //
                     suffix_list.end(),                                                  //
                     [&](std::string_view suffix) { return has_suffix(name, suffix); }); //
}

bool has_suffix(const std::filesystem::path &name, const std::vector<std::string> &suffix_list) {
  return std::any_of(suffix_list.begin(),                                                //
                     suffix_list.end(),                                                  //
                     [&](std::string_view suffix) { return has_suffix(name, suffix); }); //
}

bool is_mrtrix_image(const std::filesystem::path &path) {
  return is_dash(path.string()) || Path::has_suffix(path, {".mif", ".mih", ".mif.gz"});
}

const std::filesystem::path &home() {
  static std::filesystem::path result;
  if (result.empty()) {
    const std::optional<std::string> home = MR::get_env(home_env);
    if (!home.has_value())
      throw Exception(home_env + " environment variable is not set!");
    result = home.value();
  }
  return result;
}

} // namespace MR::Path
