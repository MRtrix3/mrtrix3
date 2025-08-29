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

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace detail {
inline void format_impl(std::ostringstream &oss, std::string_view s) { oss << s; }

// recursively replace first "{}" with the first value
template <typename T, typename... Rest>
void format_impl(std::ostringstream &oss, std::string_view string, T &&value, Rest &&...rest) {
  const auto pos = string.find("{}");
  if (pos == std::string_view::npos) {
    oss << string;
    return;
  }
  oss << string.substr(0, pos) << std::forward<T>(value);
  format_impl(oss, string.substr(pos + 2), std::forward<Rest>(rest)...);
}
} // namespace detail

namespace MR {
template <typename... Args> inline std::string format_string(std::string_view fmt, Args &&...args) {
  std::ostringstream oss;
  detail::format_impl(oss, fmt, std::forward<Args>(args)...);
  return oss.str();
}
} // namespace MR
