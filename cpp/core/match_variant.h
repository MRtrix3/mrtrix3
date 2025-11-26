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

#include <variant>

// Utility for pattern-matching style over std::variant.
// - `overload` builds an overload set from multiple lambdas (or function objects).
// - `match_v` wraps std::visit, forwarding the variant and visitors.
//   It returns whatever the selected lambda returns.
// Example:
//   std::variant<int, std::string> v = 42;
//   auto s = MR::match_v(v,
//       [](int i) { return std::to_string(i); },
//       [](const std::string& s) { return s; });
//   // s == "42"

namespace MR {
template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;
template <class var_t, class... Func> decltype(auto) match_v(var_t &&variant, Func &&...funcs) {
  return std::visit(overload{std::forward<Func>(funcs)...}, std::forward<var_t>(variant));
}
} // namespace MR
