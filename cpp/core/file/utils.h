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

#pragma once

#include <filesystem>
#include <string>

namespace MR::File {

void remove(const std::string &file);
void create(const std::string &filename, int64_t size = 0);
void resize(const std::string &filename, int64_t size);
bool is_tempfile(const std::filesystem::path &name, const char *suffix = NULL);
std::string create_tempfile(int64_t size = 0, const char *suffix = NULL);
void mkdir(const std::string &folder);
void rmdir(const std::string &folder, bool recursive = false);

} // namespace MR::File
