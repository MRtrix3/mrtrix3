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

#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "mrtrix.h"

namespace MR {

/// \brief Thread-safe wrapper around \c std::getenv().
/// \param name the name of the environment variable to query.
/// \return the value of the environment variable,
///   or an empty optional if the variable is not set.
inline std::optional<std::string> get_env(std::string_view name) {
  static std::mutex mutex;
  const std::lock_guard<std::mutex> lock(mutex);
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char *const value = std::getenv(std::string(name).c_str()); // check_syntax off
  if (value == nullptr)
    return std::nullopt;
  return std::string(value);
}

/// \brief Thread-safe wrapper around \c std::getenv() with a string default value.
/// \param name the name of the environment variable to query.
/// \param default_value the string to return if the variable is not set.
/// \return the value of the environment variable if set,
///   or \a default_value converted to \c std::string otherwise.
inline std::string get_env(std::string_view name, std::string_view default_value) {
  return get_env(name).value_or(std::string(default_value));
}

/// \brief Thread-safe wrapper around \c std::getenv() with a typed default value.
///
/// If the environment variable is set, its string value is converted to \c T via
/// \c MR::to<T>().
/// \param name the name of the environment variable to query.
/// \param default_value the value to return if the variable is not set.
/// \return the converted environment variable value if set,
///   or \a default_value otherwise.
template <typename T, typename = std::enable_if_t<!std::is_convertible_v<T, std::string_view>>>
T get_env(std::string_view name, const T &default_value) {
  const std::optional<std::string> value = get_env(name);
  return value.has_value() ? to<T>(*value) : default_value;
}

} // namespace MR
