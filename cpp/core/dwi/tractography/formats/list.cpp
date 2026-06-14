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

#include "dwi/tractography/formats/list.h"

#include <string>
#include <string_view>

#include "dwi/tractography/formats/pipe.h"
#include "dwi/tractography/formats/qfib.h"
#include "dwi/tractography/formats/tck.h"
#include "dwi/tractography/formats/trk.h"
#include "dwi/tractography/formats/trx.h"
#include "dwi/tractography/formats/tt.h"
#include "dwi/tractography/formats/vtk.h"
#include "dwi/tractography/formats/vtx.h"
#include "dwi/tractography/formats/zfib.h"

namespace MR::DWI::Tractography::Formats {

Pipe pipe_handler;
TCK tck_handler;
QFIB qfib_handler;
TRK trk_handler;
// The TRX container is serviced by three backing-specific handlers (directory /
//   uncompressed archive / compressed archive) so that the in-place sidecar-append
//   capability is fixed per handler. Their handles() predicates are disjoint
//   (an existing path resolves by backing form; a fresh ".trx" file resolves by
//   the TRXArchiveCompression config), so registration order is not load-bearing.
TRXDirectory trx_directory_handler;
TRXUncompressedArchive trx_uncompressed_handler;
TRXCompressedArchive trx_compressed_handler;
TT tt_handler;
VTK vtk_handler;
VTX vtx_handler;
ZFIB zfib_handler;

const Base *handlers[] = {&pipe_handler,
                          &tck_handler,
                          &qfib_handler,
                          &trk_handler,
                          &trx_directory_handler,
                          &trx_uncompressed_handler,
                          &trx_compressed_handler,
                          &tt_handler,
                          &vtk_handler,
                          &vtx_handler,
                          &zfib_handler,
                          nullptr};

const Base *get_handler(const std::filesystem::path &path) {
  for (const Base **format = handlers; *format != nullptr; ++format) {
    if ((*format)->handles(path))
      return *format;
  }
  return nullptr;
}

bool is_supported_extension(const std::filesystem::path &path) {
  const std::string ext = path.extension().string();
  for (const std::string_view candidate : extensions) {
    if (ext == candidate)
      return true;
  }
  return false;
}

std::string supported_extensions() {
  std::string result;
  for (const std::string_view ext : extensions) {
    if (!result.empty())
      result += ", ";
    result += ext;
  }
  return result;
}

} // namespace MR::DWI::Tractography::Formats
