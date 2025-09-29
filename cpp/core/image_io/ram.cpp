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

#include <limits>
#include <unistd.h>

#include "app.h"
#include "header.h"
#include "image_io/ram.h"

namespace MR::ImageIO {

void RAM::load(const Header &header, size_t) {
  DEBUG("allocating RAM buffer for image \"" + header.name() + "\"...");
  int64_t bytes_per_segment = (header.datatype().bits() * segsize + 7) / 8;
  addresses.resize(1);
  addresses[0].reset(new uint8_t[bytes_per_segment]);
}

} // namespace MR::ImageIO
