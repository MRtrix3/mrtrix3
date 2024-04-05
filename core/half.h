/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

namespace half_float {
class half;
}

namespace std {
template <> struct is_fundamental<half_float::half> : std::true_type {};
template <> struct is_floating_point<half_float::half> : std::true_type {};
template <> struct is_arithmetic<half_float::half> : std::true_type {};
template <> struct is_integral<half_float::half> : std::false_type {};
} // namespace std

#include "half.hpp"
