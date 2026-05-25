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

#include "file/ofstream.h"

#include <cerrno>
#include <filesystem>

#include "app.h"
#include "exception.h"
#include "file/temp.h"

namespace MR::File {

void OFStream::open(const std::filesystem::path &path, const std::ios_base::openmode mode) {
  if ((mode & std::ios_base::app) == 0 && (mode & std::ios_base::ate) == 0 && (mode & std::ios_base::in) == 0) {
    if (!File::is_tempfile(path)) {
      if (std::filesystem::exists(path)) {
        App::check_overwrite(path);
        std::filesystem::remove(path);
      }
    }
  }

  std::ofstream::open(path, mode);
  if (std::ofstream::operator!())
    throw Exception("error opening output file \"" + path.string() + "\": " + MR::C_strerror(errno));
}

} // namespace MR::File
