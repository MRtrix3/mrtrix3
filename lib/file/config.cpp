/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
      if (value.empty()) return (default_value);
      value = lowercase (value);
      if (value == "true") return (true);
      if (value == "false") return (false);
      WARN ("malformed boolean entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
      return (default_value);
    }


    int Config::get_int (const std::string& key, int default_value)
    {
      std::string value = get (key);
      if (value.empty()) return (default_value);
      try {
        return (to<int> (value));
      }
      catch (...) {
        WARN ("malformed integer entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
        return (default_value);
      }
    }


    float Config::get_float (const std::string& key, float default_value)
    {
      std::string value = get (key);
      if (value.empty()) return (default_value);
      try {
        return (to<float> (value));
      }
      catch (...) {
        WARN ("malformed floating-point entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
        return (default_value);
      }
    }


  }
}

