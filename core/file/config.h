/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __file_config_h__
#define __file_config_h__

#include <map>
#include "file/key_value.h"

namespace MR
{
  namespace File
  {
    class Config
    {
      public:

        static void init ();

        static void set (const std::string& key, const std::string& value) {
          config[key] = value;
        }
        static std::string get (const std::string& key) {
          std::map<std::string, std::string>::iterator i = config.find (key);
          return (i != config.end() ? i->second : "");
        }
        static std::string get (const std::string& key, const std::string& default_value) {
          std::map<std::string, std::string>::iterator i = config.find (key);
          return (i != config.end() ? i->second : default_value);
        }

        static void set_bool (const std::string& key, bool value) {
          set (key, (value ? "true" : "false"));
        }
        static bool get_bool (const std::string& key, bool default_value);

        static void set_int (const std::string& key, int value) {
          set (key, str (value));
        }
        static int get_int (const std::string& key, int default_value);

        static void set_float (const std::string& key, float value) {
          set (key, str (value));
        }
        static float get_float (const std::string& key, float default_value);

        static void get_RGB (const std::string& key, float* ret, float default_R, float default_G, float default_B);

      private:
        static std::map<std::string, std::string> config;
    };
  }
}

#endif

