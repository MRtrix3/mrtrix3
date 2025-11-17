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

#include <string_view>

namespace MR::File {

void remove(std::string_view filename);
void create(std::string_view filename, int64_t size = 0);
void resize(std::string_view filename, int64_t size);
bool is_tempfile(std::string_view name, std::string_view suffix = "");
std::string create_tempfile(int64_t size = 0, std::string_view suffix = "");
void mkdir(std::string_view folder);
void rmdir(std::string_view folder, bool recursive = false);

} // namespace MR::File
