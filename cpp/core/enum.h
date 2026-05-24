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
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "exception.h"

namespace MR::Enum::detail {

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

} // namespace MR::Enum::detail

namespace MR::Enum {

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

// Atomic counter array indexed by enum value.
// Provides thread-safe accumulation of per-enumerator counts without manual
// cardinality tracking or explicit atomic_init loops.
template <typename EnumType> class AtomicCounters {
  static_assert(std::is_enum_v<EnumType>, "AtomicCounters requires an enum type");

  // std::atomic is not copy- or move-constructible, so a std::array of
  // std::atomic cannot be value-initialised through aggregate brace init.
  // Wrapping it gives a default constructor that zero-initialises the value.
  // The atomic is marked mutable so that thread-safe accumulation can be
  // exposed through const member functions of the enclosing helper.
  struct Counter {
    mutable std::atomic<size_t> value;
    Counter() noexcept : value(0) {}
  };

  static constexpr size_t N = magic_enum::enum_count<EnumType>();
  std::array<Counter, N> counters;

public:
  AtomicCounters() = default;
  AtomicCounters(const AtomicCounters &) = delete;
  AtomicCounters &operator=(const AtomicCounters &) = delete;

  static constexpr size_t size() noexcept { return N; }

  void add(EnumType value, size_t increment = 1) const noexcept {
    counters[*magic_enum::enum_index(value)].value.fetch_add(increment, std::memory_order_relaxed);
  }

  size_t get(EnumType value) const noexcept {
    return counters[*magic_enum::enum_index(value)].value.load(std::memory_order_seq_cst);
  }

  size_t total() const noexcept {
    size_t sum = 0;
    for (const auto &counter : counters)
      sum += counter.value.load(std::memory_order_seq_cst);
    return sum;
  }

  template <typename Callable> void for_each(Callable &&callable) const {
    constexpr auto values = magic_enum::enum_values<EnumType>();
    for (size_t i = 0; i != N; ++i)
      std::forward<Callable>(callable)(values[i], counters[i].value.load(std::memory_order_seq_cst));
  }
};

// Converts a string to the corresponding enum value, ignoring case.
// If no matching enum value is found, throws an exception.
template <typename EnumType> inline EnumType from_name(std::string_view enum_name) {
  const auto value = magic_enum::enum_cast<EnumType>(enum_name, magic_enum::case_insensitive);
  if (!value.has_value()) {
    throw Exception("Unsupported value '{}'. Supported values are: {}",
                    enum_name,
                    detail::join(lower_case_names<EnumType>(), ", "));
  }
  return value.value();
}

} // namespace MR::Enum
