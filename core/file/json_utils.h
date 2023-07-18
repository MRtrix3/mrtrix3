/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __file_json_utils_h__
#define __file_json_utils_h__

#include "file/json.h"
#include "file/key_value.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace JSON
    {

      void load (Header& H, const std::string& path);
      void save (const Header& H, const std::string& json_path, const std::string& image_path);

      KeyValues read (const nlohmann::json& json,
                      const KeyValues& preexisting = KeyValues());
      void read (const nlohmann::json& json,
                 Header& header,
                 const bool realign);

      void write (const KeyValues& keyval, nlohmann::json& json);
      void write (const Header& header,
                  nlohmann::json& json,
                  const std::string& image_path);

    }
  }
}

#endif

