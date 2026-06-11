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

#include <filesystem>

#include "dwi/tractography/formats/base.h"

//! Classes responsible for handling of specific tractography formats
namespace MR::DWI::Tractography::Formats {

/*! a null-terminated, ordered list of all handlers for supported tractography
 * formats. Format selection loops this array; the first handler whose
 * handles() recognises the path wins (cf. MR::Formats::handlers). */
extern const Base *handlers[];

//! \brief select the format handler responsible for \a path.
/*! Loops the handler list and returns the first handler that recognises
 * \a path (by extension). Returns nullptr if no handler matches; callers
 * raise a user-interpretable error in that case. */
const Base *get_handler(const std::filesystem::path &path);

} // namespace MR::DWI::Tractography::Formats
