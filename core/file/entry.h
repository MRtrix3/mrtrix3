/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __file_entry_h__
#define __file_entry_h__

#include <string>

#include "types.h"

namespace MR
{
  namespace File
  {

    class Entry { NOMEMALIGN
      public:
        Entry (const std::string& fname, int64_t offset = 0) :
          name (fname), start (offset) { }

        Entry (const Entry&) = default;
        Entry (Entry&&) noexcept = default;
        Entry& operator=(Entry&& E) noexcept {
          name = std::move (E.name);
          start = E.start;
          return *this;
        }

        std::string name;
        int64_t start;
    };


    inline std::ostream& operator<< (std::ostream& stream, const Entry& e)
    {
      stream << "File::Entry { \"" << e.name << "\", offset " << e.start << " }";
      return stream;
    }
  }
}

#endif

