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

namespace MR::File {

void create(const std::filesystem::path &path, int64_t size = 0);
void remove(const std::filesystem::path &path);
void resize(const std::filesystem::path &path, int64_t size);
void mkdir(const std::filesystem::path &folder);
void rmdir(const std::filesystem::path &folder, bool recursive = false);

} // namespace MR::File
