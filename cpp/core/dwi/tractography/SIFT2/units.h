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

#include <string>
#include <vector>

namespace MR::DWI::Tractography::SIFT2 {

const std::vector<std::string> units_choices = {"NOS", "none", "AFD/mm", "AFD.mm-1", "AFD.mm^-1", "mm2", "mm^2"};

enum class units_t { NOS, AFDpermm, mm2 };

constexpr units_t default_units = units_t::mm2;

units_t str2units(const std::string &);
std::string units2str(units_t);

} // namespace MR::DWI::Tractography::SIFT2
