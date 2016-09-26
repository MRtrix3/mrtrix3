/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include <fstream>

#include "file/json_utils.h"

#include "exception.h"
#include "header.h"
#include "mrtrix.h"
#include "file/ofstream.h"

namespace MR
{
  namespace File
  {
    namespace JSON
    {



      void load (Header& H, const std::string& path)
      {
        std::ifstream in (path);
        if (!in)
          throw Exception ("Error opening JSON file \"" + path + "\"");
        nlohmann::json json;
        try {
          in >> json;
        } catch (std::logic_error& e) {
          throw Exception ("Error parsing JSON file \"" + path + "\": " + e.what());
        }
        for (auto i = json.cbegin(); i != json.cend(); ++i) {

          if (i->is_boolean()) {
            H.keyval().insert (std::make_pair (i.key(), i.value() ? "1" : "0"));
          } else if (i->is_number_integer()) {
            H.keyval().insert (std::make_pair (i.key(), str<int>(i.value())));
          } else if (i->is_number_float()) {
            H.keyval().insert (std::make_pair (i.key(), str<float>(i.value())));
          } else if (i->is_array()) {
            std::vector<std::string> s;
            for (auto j = i->cbegin(); j != i->cend(); ++j)
              s.push_back (str(*j));
            H.keyval().insert (std::make_pair (i.key(), join(s, "\n")));
          } else if (i->is_string()) {
            const std::string s = i.value();
            H.keyval().insert (std::make_pair (i.key(), s));
          }

        }
      }



      void save (const Header& H, const std::string& path)
      {
        nlohmann::json json;
        for (const auto& kv : H.keyval())
          json[kv.first] = kv.second;
        File::OFStream out (path);
        out << json.dump(4);
      }



    }
  }
}

