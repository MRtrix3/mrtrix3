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

#include "file/ofstream.h"

#include "exception.h"
#include "file/utils.h"

namespace MR::File {

void OFStream::open(const std::string &path, const std::ios_base::openmode mode) {
  if (!(mode & std::ios_base::app) && !(mode & std::ios_base::ate) && !(mode & std::ios_base::in)) {
    if (!File::is_tempfile(path))
      File::create(path);
  }

  std::ofstream::open(path.c_str(), mode);
  if (std::ofstream::operator!())
    throw Exception("error opening output file \"" + path + "\": " + std::strerror(errno));
}

} // namespace MR::File
