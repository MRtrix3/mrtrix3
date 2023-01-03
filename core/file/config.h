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

#ifndef __file_config_h__
#define __file_config_h__

#include <map>
#include "types.h"
#include "file/key_value.h"

namespace MR
{
  namespace File
  {
    class Config { NOMEMALIGN
      public:

        static void init ();

        static void set (const std::string& key, const std::string& value) {
          config[key] = value;
        }
        static std::string get (const std::string& key) {
          const KeyValues::const_iterator i = config.find (key);
          return (i != config.end() ? i->second : "");
        }
        static std::string get (const std::string& key, const std::string& default_value) {
          KeyValues::iterator i = config.find (key);
          return (i != config.end() ? i->second : default_value);
        }

        static bool  get_bool  (const std::string& key, bool default_value);
        static int   get_int   (const std::string& key, int default_value);
        static float get_float (const std::string& key, float default_value);
        static void  get_RGB   (const std::string& key, float* ret, float default_R, float default_G, float default_B);

      private:
        static KeyValues config;
    };
  }
}

#endif

