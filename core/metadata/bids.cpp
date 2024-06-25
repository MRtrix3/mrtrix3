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

#include "metadata/bids.h"

#include "exception.h"
#include "mrtrix.h"

namespace MR::Metadata::BIDS {

std::string vector2axisid(const axis_vector_type &dir) {
  if (dir[0] == -1) {
    assert(!dir[1]);
    assert(!dir[2]);
    return "i-";
  } else if (dir[0] == 1) {
    assert(!dir[1]);
    assert(!dir[2]);
    return "i";
  } else if (dir[1] == -1) {
    assert(!dir[0]);
    assert(!dir[2]);
    return "j-";
  } else if (dir[1] == 1) {
    assert(!dir[0]);
    assert(!dir[2]);
    return "j";
  } else if (dir[2] == -1) {
    assert(!dir[0]);
    assert(!dir[1]);
    return "k-";
  } else if (dir[2] == 1) {
    assert(!dir[0]);
    assert(!dir[1]);
    return "k";
  } else {
    throw Exception("Malformed image axis vector: \"" + str(dir.transpose()) + "\"");
  }
}

axis_vector_type axisid2vector(const std::string &id) {
  if (id == "i-")
    return {-1, 0, 0};
  else if (id == "i")
    return {1, 0, 0};
  else if (id == "j-")
    return {0, -1, 0};
  else if (id == "j")
    return {0, 1, 0};
  else if (id == "k-")
    return {0, 0, -1};
  else if (id == "k")
    return {0, 0, 1};
  else
    throw Exception("Malformed image axis identifier: \"" + id + "\"");
}

} // namespace MR::Metadata::BIDS
