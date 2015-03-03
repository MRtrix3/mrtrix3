/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Maximilian Pietsch, 03/03/15.

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

// #include "image/stride.h"
// #include "get_set.h"
// #include "file/config.h"
#include "file/par_utils.h"
// #include "math/LU.h"
// #include "math/permutation.h"
// #include "math/versor.h"
// #include "image/header.h"

namespace MR
{
  namespace File
  {
    namespace PAR
    {
      std::string KeyValue::trim(std::string const& str)
      {
        if (str.empty())
            return str;
        if (str[0] == '.')
          return trim(str.substr(1, str.find_last_not_of(' ')));
        std::string whitespaces (" \t\f\v\n\r");
        size_t first = str.find_first_not_of(whitespaces);
        size_t last  = str.find_last_not_of(whitespaces);
        return str.substr(first, last-first+1);
      }


      void KeyValue::open (const std::string& file, const char* first_line)
      {
        filename.clear();
        DEBUG ("reading key/value file \"" + file + "\"...");

        in.open (file.c_str(), std::ios::in | std::ios::binary);
        if (!in) 
          throw Exception ("failed to open key/value file \"" + file + "\": " + strerror (errno));
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

      bool KeyValue::next_general ()
      {
        while (in.good() && general_information ) {
          std::string sbuf;
          getline (in, sbuf);
          if (in.bad()) 
            throw Exception ("error reading PAR file \"" + filename + "\": " + strerror (errno));

            if (sbuf.find("IMAGE INFORMATION") != std::string::npos){
              DEBUG ("general_information = false: after line (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
              general_information = false;
              return true;
            }
            if (!ver.size() && (sbuf.find("image export tool")!= std::string::npos) ){
              ver = KeyValue::trim(sbuf.substr (sbuf.find_last_of("image export tool") ) );
              DEBUG("par/rec version: " + ver);
              return true;
            }

          sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
          if (sbuf.size()) {
            size_t colon = sbuf.find_first_of (':');
            if (colon == std::string::npos) {
              INFO ("malformed key/value entry (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
              return true;
            }
            else {
              K = KeyValue::trim(strip (sbuf.substr (0, colon)));
              V = KeyValue::trim(strip (sbuf.substr (colon+1)));
              if (K.empty()) {
                INFO ("malformed key/value entry (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
              }
              else 
                return true;
            }
          }
        }
        return false;
      }

      bool KeyValue::next_image ()
      {
        while (in.good() && !general_information ) {
          std::string sbuf;
          getline (in, sbuf);
          if (in.bad()) 
            throw Exception ("error reading PAR file \"" + filename + "\": " + strerror (errno));

          if (sbuf.find("END OF DATA DESCRIPTION FILE") != std::string::npos) {
            DEBUG ("END OF DATA DESCRIPTION FILE");
            in.setstate (std::ios::eofbit);
            return false;
          }
          sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
          if (sbuf.size()) {
            K = "image";
            V = sbuf;
            return true;
          }
        }
        return false;
      }



          //  else if (sbuf.size()){
          //   WARN(sbuf);
          // }

      // size_t read (Image::Header& H, const par_header& NH) {
      //   bool is_BE = false;
      //   size_t data_offset; // = (size_t) get<float32> (&NH.vox_offset, is_BE);
      //   return data_offset;
      // }
      
    }
  }
}


