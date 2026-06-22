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

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

#include "dwi/tractography/formats/base.h"

//! Classes responsible for handling of specific tractography formats
namespace MR::DWI::Tractography::Formats {

/*! a null-terminated, ordered list of all handlers for supported tractography
 * formats. Format selection loops this array; the first handler whose
 * handles() recognises the path wins (cf. MR::Formats::handlers). */
extern const Base *handlers[];

//! \brief every filename extension (each including the leading '.') recognised
//!   by the tractography format handlers, in handler order.
/*! The companion to handlers[]: where handlers[] drives runtime format
 * selection, this list enumerates the extensions a command-line tractogram
 * argument may carry, so that argument parsing (App::Argument::type_tracks_in()
 * / type_tracks_out()) can accept the full range of supported formats rather
 * than ".tck" alone. The directory-backed TRX dataset and the inter-command
 * pipe carry no extension and are handled separately by the caller. */
inline constexpr std::array<std::string_view, 8> extensions{
    ".tck", ".qfib", ".trk", ".trx", ".tt", ".vtk", ".vtx", ".zfib"};

//! \brief select the format handler responsible for \a path.
/*! Loops the handler list and returns the first handler that recognises
 * \a path (by extension). Returns nullptr if no handler matches; callers
 * raise a user-interpretable error in that case. */
const Base *get_handler(const std::filesystem::path &path);

//! \brief whether \a path bears a filename extension recognised by a handler.
/*! A pure path test against `extensions`, case-sensitive to match the
 * per-handler handles() tests; does not touch the filesystem. The directory
 * form of a tractogram path (a TRX dataset) carries no extension and is tested
 * separately by the caller. */
bool is_supported_extension(const std::filesystem::path &path);

//! \brief whether \a path unambiguously designates a directory-backed tractogram
//!   output dataset (currently only the TRX format is directory-backed).
/*! A tractography output is written as a directory exactly when the directory
 * intent is unambiguous: the path ends with a directory separator, or it already
 * exists as an empty directory. A bare extensionless name is deliberately left
 * ambiguous (it may equally name a regular file) and is NOT treated as a
 * directory. Consulted both by the command-line parser, to apply directory-output
 * validation to an ArgTypeTracksOut argument, and by the directory handler's path
 * test. Touches the filesystem only to resolve the existing-empty-directory case. */
bool is_directory_dataset_output(const std::filesystem::path &path);

//! \brief the recognised extensions as a comma-separated string for messages.
std::string supported_extensions();

} // namespace MR::DWI::Tractography::Formats
