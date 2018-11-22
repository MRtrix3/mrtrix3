/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __file_key_value_h__
#define __file_key_value_h__

#include <fstream>
#include "mrtrix.h"

namespace MR
{
  namespace File
  {

    class KeyValue { NOMEMALIGN
      public:
        KeyValue () { }
        KeyValue (const std::string& file, const char* first_line = NULL) {
          open (file, first_line);
        }

        void  open (const std::string& file, const char* first_line = NULL);
        bool  next ();
        void  close () {
          in.close();
        }

        const std::string& key () const throw ()   {
          return (K);
        }
        const std::string& value () const throw () {
          return (V);
        }
        const std::string& name () const throw ()  {
          return (filename);
        }

      protected:
        std::string K, V, filename;
        std::ifstream in;
    };

  }
}

#endif

