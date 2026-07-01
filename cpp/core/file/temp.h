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

#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace MR::File {

bool is_tempfile(const std::filesystem::path &name, std::string_view suffix = "");
std::filesystem::path create_tempfile(int64_t size = 0, std::string_view suffix = "");

//! \brief create an empty temporary directory alongside create_tempfile()'s files.
/*! The directory is created in the same location used by create_tempfile()
 * (the TmpFileDir config / MRTRIX_TMPFILE_DIR location), with the same
 * randomised TmpFilePrefix basename, plus the optional \a suffix. The caller
 * owns the directory's lifetime: it should register it with the signal handler
 * for cleanup on unexpected termination (SignalHandler::mark_file_for_deletion)
 * and remove it once finished. Used by the TRX handler to extract a compressed
 * archive before memory-mapping its members (D5). */
std::filesystem::path create_tempdir(std::string_view suffix = "");

} // namespace MR::File
