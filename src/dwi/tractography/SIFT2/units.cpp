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

#include "dwi/tractography/SIFT2/units.h"

#include "exception.h"
#include "mrtrix.h"

namespace MR::DWI::Tractography::SIFT2 {

extern const char* const units_choices[] = { "NOS", "none", "AFD/mm", "AFD.mm-1", "AFD.mm^-1", "mm2", "mm^2", nullptr };

units_t str2units(const std::string& s) {
  const std::string slower = lowercase(s);
  if (slower == "nos" || slower == "none")
    return units_t::NOS;
  if (slower == "afd/mm" || slower == "afd.mm-1" || slower == "afd.mm^-1")
    return units_t::AFDpermm;
  if (slower == "mm2" || slower == "mm^2")
    return units_t::mm2;
  throw Exception("Unable to convert string \"" + s + "\" to SIFT2 streamline weight units");
}

std::string units2str(units_t units) {
  switch (units) {
    case units_t::NOS: return "NOS";
    case units_t::AFDpermm: return "AFD/mm";
    case units_t::mm2: return "mm^2";
  }
  throw Exception("Unexpected units provided to SIFT2::units2str()");
}

} // namespace MR::DWI::Tractography::SIFT2
