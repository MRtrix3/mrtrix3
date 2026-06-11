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

#include "dwi/tractography/sidecar.h"

#include <string>
#include <string_view>

namespace MR::DWI::Tractography {

SidecarReference parse_sidecar_reference(std::string_view arg) {
  // §2.4: split on the LAST "::". A bare path (no "::", including a Windows
  //   "C:\\..." drive-letter path which carries only single colons) yields a
  //   reference with no field name; "DATASET::NAME" yields both components.
  const std::string_view::size_type pos = arg.rfind("::");
  if (pos == std::string_view::npos)
    return SidecarReference{std::filesystem::path(std::string(arg)), std::nullopt};
  return SidecarReference{std::filesystem::path(std::string(arg.substr(0, pos))), std::string(arg.substr(pos + 2))};
}

} // namespace MR::DWI::Tractography
