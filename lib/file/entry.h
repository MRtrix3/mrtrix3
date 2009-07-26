/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 24/07/09.

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

#ifndef __file_entry_h__
#define __file_entry_h__

#include <string>

namespace MR {
  namespace File {

    class Entry {
      public:
        Entry (const std::string& fname, bool read_write = false, off64_t from = 0) :
          name (fname), start (from), readwrite (read_write) { }

        std::string name;
        off64_t start;
        bool readwrite;
    };


    inline std::ostream& operator<< (std::ostream& stream, const Entry& e) {
      stream << "File::Entry { \"" << e.name << "\" "
        << ( e.readwrite ? "RW" : "RO" ) << " from offset " << e.start << " }";
      return (stream);
    }
  }
}

#endif

