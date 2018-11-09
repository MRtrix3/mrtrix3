/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "debug.h"

#include "file/path.h"
#include "file/config.h"

#define MRTRIX_CONFIG_FILE "mrtrix.conf"
#define MRTRIX_SYS_CONFIG_FILE "/etc/" MRTRIX_CONFIG_FILE
#define MRTRIX_USER_CONFIG_FILE "." MRTRIX_CONFIG_FILE



namespace MR
{
  namespace File
  {

    std::map<std::string, std::string> Config::config;

    void Config::init ()
    {
      const char* sysconf_location = getenv ("MRTRIX_CONFIGFILE");
      if (!sysconf_location) 
        sysconf_location = MRTRIX_SYS_CONFIG_FILE;

      if (Path::is_file (sysconf_location)) {
        INFO (std::string("reading config file \"") + sysconf_location + "\"...");
        try {
          KeyValue kv (sysconf_location);
          while (kv.next()) {
            config[kv.key()] = kv.value();
          }
        }
        catch (...) { }
      } else {
        DEBUG (std::string ("No config file found at \"") + sysconf_location + "\"");
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
      } else {
        DEBUG ("No config file found at \"" + path + "\"");
      }
    }



    bool Config::get_bool (const std::string& key, bool default_value)
    {
      std::string value = get (key);
      if (value.empty()) 
        return default_value;
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



    void Config::get_RGB (const std::string& key, float* ret, float default_R, float default_G, float default_B)
    {
      std::string value = get (key);
      if (value.size()) {
        try {
          vector<default_type> V (parse_floats (value));
          if (V.size() < 3) 
            throw Exception ("malformed RGB entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
          ret[0] = V[0];
          ret[1] = V[1];
          ret[2] = V[2];
        }
        catch (Exception) { }
      }
      else {
        ret[0] = default_R;
        ret[1] = default_G;
        ret[2] = default_B;
      }
    }


  }
}

