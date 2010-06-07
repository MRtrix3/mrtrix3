/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.


    08-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fixed get_int() & get_float()
      They were previously declared as returning bool

*/

#ifndef __file_config_h__
#define __file_config_h__

#include <map>
#include "file/key_value.h"

namespace MR {
  namespace File {
    class Config {
      public:

        static void      init ();

        static void      set (const std::string& key, const std::string& value) { config[key] = value; } 
        static std::string    get (const std::string& key)
        {
          std::map<std::string, std::string>::iterator i = config.find (key);
          return (i != config.end() ? i->second : "");
        }

        static void      set_bool (const std::string& key, bool value) { set (key, ( value ? "true" : "false" )); }
        static bool      get_bool (const std::string& key, bool default_value);

        static void      set_int (const std::string& key, int value) { set (key, str (value)); }
        static int       get_int (const std::string& key, int default_value);

        static void      set_float (const std::string& key, float value) { set (key, str (value)); }
        static float     get_float (const std::string& key, float default_value);

      private:
        static std::map<std::string, std::string> config;
    };
  }
}

#endif

