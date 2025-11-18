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

#include "header.h"
#include "image_io/pipe.h"
#include "signal_handler.h"

namespace MR::ImageIO {

void Pipe::load(const Header &header, size_t) {
  assert(files.size() == 1);
  DEBUG("mapping piped image \"" + files[0].name + "\"...");

  int64_t bytes_per_segment = (header.datatype().bits() * segsize + 7) / 8;

  mmap.reset(new File::MMap(files[0], writable, !is_new, bytes_per_segment));
  addresses.resize(1);
  addresses[0].reset(mmap->address());
}

void Pipe::unload(const Header &) {
  if (mmap) {
    mmap.reset();
    if (is_new) {
      std::cout << files[0].name << "\n";
      SignalHandler::unmark_file_for_deletion(files[0].name);
    }
    addresses[0].release();
  }
}

// ENVVAR name: MRTRIX_PRESERVE_TMPFILE
// ENVVAR This variable decides whether the temporary piped image
// ENVVAR should be preserved rather than the usual behaviour of
// ENVVAR deletion at command completion.
// ENVVAR For example, in case of piped commands from Python API,
// ENVVAR it is necessary to retain the temp files until all
// ENVVAR the piped commands are executed.
namespace {
bool preserve_tmpfile() {
  const char *const MRTRIX_PRESERVE_TMPFILE = getenv("MRTRIX_PRESERVE_TMPFILE"); // check_syntax off
  return (MRTRIX_PRESERVE_TMPFILE != nullptr && to<bool>(std::string(MRTRIX_PRESERVE_TMPFILE)));
}
} // namespace
bool Pipe::delete_piped_images = !preserve_tmpfile();

} // namespace MR::ImageIO
