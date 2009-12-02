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

#include <fstream>
#include "file/key_value.h"


namespace MR {
  namespace File {

    void KeyValue::open (const std::string& file, const char* first_line)
    {
      filename.clear();
      debug ("reading key/value file \"" + file + "\"...");

      in.open (file.c_str(), std::ios::in | std::ios::binary);
      if (!in) throw Exception ("failed to open key/value file \"" + file + "\": " + strerror(errno));
      if (first_line) {
        std::string sbuf;
        getline (in, sbuf);
        if (sbuf.compare (0, strlen (first_line), first_line)) {
          in.close();
          throw Exception ("invalid first line for key/value file \"" + file + "\" (expected \"" + first_line + "\")");
        }
      }
      filename = file;
    }





    bool KeyValue::next ()
    {
      while (in.good()) {
        std::string sbuf;
        getline (in, sbuf);
        if (in.bad()) throw Exception ("error reading key/value file \"" + filename + "\": " + strerror (errno));

        sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
        if (sbuf == "END") {
          in.setstate (std::ios::eofbit);
          return (false);
        }

        if (sbuf.size()) {
          size_t colon = sbuf.find_first_of (':');
          if (colon == std::string::npos) info ("WARNING: malformed key/value entry (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
          else {
            K = strip (sbuf.substr (0, colon));
            V = strip (sbuf.substr (colon+1));
            if (K.empty() || V.empty()) info ("WARNING: malformed key/value entry (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
            else return (true);
          }

        }
      }
      return (false);
    }



  }
}


