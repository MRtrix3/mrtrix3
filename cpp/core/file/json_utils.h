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

#include "file/key_value.h"
#include <nlohmann/json.hpp>

namespace MR {
class Header;
}

namespace MR::File::JSON {

void load(Header &H, std::string_view path);
void save(const Header &H, std::string_view json_path, std::string_view image_path);

KeyValues read(const nlohmann::json &json);
void read(const nlohmann::json &json, Header &header);

void write(const KeyValues &keyval, nlohmann::json &json);
void write(const Header &header, nlohmann::json &json, std::string_view image_path);

} // namespace MR::File::JSON
