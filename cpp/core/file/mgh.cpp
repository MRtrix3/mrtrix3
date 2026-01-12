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

#include <cctype>

#include "file/mgh.h"
#include "file/ofstream.h"
#include "header.h"

namespace MR::File::MGH {

std::string tag_ID_to_string(const tag_type tag) {
  try {
    return tag2str.at(tag);
  } catch (std::out_of_range &) {
    return "MGH_TAG_" + str(tag);
  }
}

tag_type string_to_tag_ID(std::string_view key) {
  if (key.compare(0, 8, "MGH_TAG_") != 0)
    return 0;

  for (const auto &i : tag2str) {
    if (i.second == key)
      return i.first;
  }
  return 0;
}

} // namespace MR::File::MGH
