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

