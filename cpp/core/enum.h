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

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "exception.h"

namespace MR::Enum {

namespace detail {

inline std::string lowercase(std::string_view string) {
  std::string result(string);
  std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return result;
}

inline std::string join(const std::vector<std::string> &values, std::string_view delimiter) {
  if (values.empty())
    return {};

  std::string result(values[0]);
  for (auto i = values.begin() + 1; i != values.end(); ++i)
    result += std::string(delimiter) + *i;
  return result;
}

} // namespace detail

// Returns a vector of the lowercase names of the enum values, in the order they are defined in the enum.
template <typename EnumType> inline std::vector<std::string> lower_case_names() {
  static constexpr auto names = magic_enum::enum_names<EnumType>();
  std::vector<std::string> result;
  result.reserve(names.size());
  for (const auto &name : names)
    result.push_back(detail::lowercase(name));
  return result;
}

// Returns a concatenated string of the enum lowercase names, separated by the specified delimiter.
// Default delimiter is a comma followed by a space.
template <typename EnumType> inline std::string join(std::string_view delimiter = ", ") {
  return detail::join(lower_case_names<EnumType>(), delimiter);
}

// Returns the case-sensitive name of the enum value.
template <typename EnumType> inline std::string name(EnumType value) {
  return std::string(magic_enum::enum_name(value));
}

// Returns the lowercase name of the enum value.
template <typename EnumType> inline std::string lowercase_name(EnumType value) {
  return detail::lowercase(magic_enum::enum_name(value));
}

// Converts a string to the corresponding enum value, ignoring case.
// If no matching enum value is found, throws an exception.
template <typename EnumType> inline EnumType from_name(std::string_view enum_name) {
  const auto value = magic_enum::enum_cast<EnumType>(enum_name, magic_enum::case_insensitive);
  if (!value.has_value()) {
    const std::string error = "Unsupported value '" + std::string(enum_name) +
                              "'. Supported values are: " + detail::join(lower_case_names<EnumType>(), ", ");
    throw Exception(error);
  }
  return value.value();
}

} // namespace MR::Enum
