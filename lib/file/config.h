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

      private:
        static std::map<std::string, std::string> config;
    };
  }
}

#endif

