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

*/

#include "file/path.h"
#include "file/config.h"

#define MRTRIX_CONFIG_FILE "mrtrix.conf"

#ifdef MRTRIX_WINDOWS
#define MRTRIX_SYS_CONFIG_FILE "C:\\" MRTRIX_CONFIG_FILE
#define MRTRIX_USER_CONFIG_FILE MRTRIX_CONFIG_FILE
#else
#define MRTRIX_SYS_CONFIG_FILE "/etc/" MRTRIX_CONFIG_FILE
#define MRTRIX_USER_CONFIG_FILE "." MRTRIX_CONFIG_FILE
#endif



namespace MR
{
  namespace File
  {

    std::map<std::string, std::string> Config::config;

    void Config::init ()
    {
      if (Path::is_file (MRTRIX_SYS_CONFIG_FILE)) {
        INFO ("reading config file \"" MRTRIX_SYS_CONFIG_FILE "\"...");
        try {
          KeyValue kv (MRTRIX_SYS_CONFIG_FILE);
          while (kv.next()) {
            config[kv.key()] = kv.value();
          }
        }
        catch (...) { }
      }

      std::string path = Path::join (Path::home(), MRTRIX_USER_CONFIG_FILE);
      if (Path::is_file (path)) {
        INFO ("reading config file \"" + path + "\"...");
        try {
          KeyValue kv (path);
          while (kv.next()) {
            config[kv.key()] = kv.value();
          }
        }
        catch (...) { }
      }
    }



    bool Config::get_bool (const std::string& key, bool default_value)
    {
      std::string value = get (key);
      if (value.empty()) 
        return default_value;
      value = lowercase (value);
      if (value == "true") 
        return true;
      if (value == "false") 
        return false;
      try {
        return to<bool> (value);
      }
      catch (...) {
        WARN ("malformed boolean entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
        return default_value;
      }
    }


    int Config::get_int (const std::string& key, int default_value)
    {
      std::string value = get (key);
      if (value.empty()) 
        return default_value;
      try {
        return to<int> (value);
      }
      catch (...) {
        WARN ("malformed integer entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
        return default_value;
      }
    }


    float Config::get_float (const std::string& key, float default_value)
    {
      std::string value = get (key);
      if (value.empty()) 
        return default_value;
      try {
        return to<float> (value);
      }
      catch (...) {
        WARN ("malformed floating-point entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
        return default_value;
      }
    }


  }
}

