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

#include "image_io/null.h"
#include "header.h"

namespace MR::ImageIO {

void Null::load(const Header &header, size_t) {
  throw Exception("No suitable handler to access data in \"" + header.name() + "\"");
}

void Null::unload(const Header &header) {
  throw Exception("No suitable handler to access data in \"" + header.name() + "\"");
}

} // namespace MR::ImageIO
