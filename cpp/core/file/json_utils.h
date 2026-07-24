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
#include <nlohmann/json.hpp>

#include "file/key_value.h"

namespace MR {
class Header;
}

namespace MR::File::JSON {

//! whether a JSON sidecar should retain or strip the DW gradient scheme carried in the header
/*! The scheme is stripped when the invoking command is exporting it to a separate file
 *  (-export_grad_*), so it is not duplicated in the sidecar; retained otherwise. The decision is
 *  supplied by the command — which alone knows whether it offers gradient export — so the generic
 *  JSON writer never queries the command-line (and thus never probes an option a command may not offer). */
enum class DWScheme { Retain, Strip };

void load(Header &H, const std::filesystem::path &path);
void save(const Header &H,
          const std::filesystem::path &json_path,
          const std::filesystem::path &image_path,
          DWScheme dw_scheme = DWScheme::Retain);

KeyValues read(const nlohmann::json &json);
void read(const nlohmann::json &json, Header &header);

void write(const KeyValues &keyval, nlohmann::json &json);
void write(const Header &header,
           nlohmann::json &json,
           const std::filesystem::path &image_path,
           DWScheme dw_scheme = DWScheme::Retain);

} // namespace MR::File::JSON
