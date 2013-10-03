/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 31/07/13.

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


#include "image/format/mrtrix_utils.h"


namespace MR
{
  namespace Image
  {
    namespace Format
    {




      bool next_keyvalue (File::KeyValue& kv, std::string& key, std::string& value)
      {
        key.clear(); value.clear();
        if (!kv.next())
          return false;
        key = kv.key();
        value = kv.value();
        return true;
      }


      bool next_keyvalue (File::GZ& gz, std::string& key, std::string& value)
      {
        key.clear(); value.clear();
        std::string line = gz.getline();
        line = strip (line.substr (0, line.find_first_of ('#')));
        if (line.empty() || line == "END")
          return false;

        size_t colon = line.find_first_of (':');
        if (colon == std::string::npos) {
          INFO ("malformed key/value entry (\"" + line + "\") in file \"" + gz.name() + "\" - ignored");
        } else {
          key   = strip (line.substr (0, colon));
          value = strip (line.substr (colon+1));
          if (key.empty() || value.empty()) {
            INFO ("malformed key/value entry (\"" + line + "\") in file \"" + gz.name() + "\" - ignored");
            key.clear();
            value.clear();
          }
        }
        return true;
      }






      void get_mrtrix_file_path (Header& H, const std::string& flag, std::string& fname, size_t& offset)
      {

        Header::iterator i = H.find (flag);
        if (i == H.end())
          throw Exception ("missing \"" + flag + "\" specification for MRtrix image \"" + H.name() + "\"");
        const std::string path = i->second;
        H.erase (i);

        std::istringstream file_stream (path);
        file_stream >> fname;
        offset = 0;
        if (file_stream.good()) {
          try {
            file_stream >> offset;
          }
          catch (...) {
            throw Exception ("invalid offset specified for file \"" + fname
                             + "\" in MRtrix image header \"" + H.name() + "\"");
          }
        }

        if (fname == ".") {
          if (offset == 0)
            throw Exception ("invalid offset specified for embedded MRtrix image \"" + H.name() + "\"");
          fname = H.name();
        } else {
          fname = Path::join (Path::dirname (H.name()), fname);
        }

      }




    }
  }
}
