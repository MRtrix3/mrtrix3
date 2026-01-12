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

#include "image_io/ram.h"
#include "file/path.h"
#include "formats/list.h"
#include "header.h"

namespace MR::Formats {

std::unique_ptr<ImageIO::Base> RAM::read(Header &H) const {
#ifdef MRTRIX_AS_R_LIBRARY
  if (!Path::has_suffix(H.name(), ".R"))
    return std::unique_ptr<ImageIO::Base>();

  Header *R_header = (Header *)to<size_t>(H.name().substr(0, H.name().size() - 2));
  H = *R_header;
  return R_header->__get_handler();
#else
  return {};
#endif
}

bool RAM::check(Header &H, size_t num_axes) const {
  return H.name() == "NULL"
#ifdef MRTRIX_AS_R_LIBRARY
         || Path::has_suffix(H.name(), ".R")
#endif
      ;
}

std::unique_ptr<ImageIO::Base> RAM::create(Header &H) const {
  if (H.name() == "NULL") {
    std::unique_ptr<ImageIO::RAM> io_handler(new ImageIO::RAM(H));
    return io_handler;
  }

#ifdef MRTRIX_AS_R_LIBRARY
  Header *R_header = (Header *)to<size_t>(H.name().substr(0, H.name().size() - 2));
  *R_header = H;
  std::unique_ptr<ImageIO::RAM> io_handler(new ImageIO::RAM(H));
  R_header->__set_handler(io_handler);
  return io_handler;
#else
  return {};
#endif
}

} // namespace MR::Formats
