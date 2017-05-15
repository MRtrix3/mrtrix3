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


#include "formats/mrtrix_utils.h"


namespace MR
{
  namespace Formats
  {



    vector<ssize_t> parse_axes (size_t ndim, const std::string& specifier)
    {
      vector<ssize_t> parsed (ndim);

      size_t sub = 0;
      size_t lim = 0;
      size_t end = specifier.size();
      size_t current = 0;

      try {
        while (sub <= end) {
          bool pos = true;
          if (specifier[sub] == '+') {
            pos = true;
            sub++;
          }
          else if (specifier[sub] == '-') {
            pos = false;
            sub++;
          }
          else if (!isdigit (specifier[sub])) throw 0;

          lim = sub;

          while (isdigit (specifier[lim])) lim++;
          if (specifier[lim] != ',' && specifier[lim] != '\0') throw 0;
          parsed[current] = to<ssize_t> (specifier.substr (sub, lim-sub)) + 1;
          if (!pos) parsed[current] = -parsed[current];

          sub = lim+1;
          current++;
        }
      }
      catch (int) {
        throw Exception ("malformed axes specification \"" + specifier + "\"");
      }

      if (current != ndim)
        throw Exception ("incorrect number of axes in axes specification \"" + specifier + "\"");

      if (parsed.size() != ndim)
        throw Exception ("incorrect number of dimensions for axes specifier");
      for (size_t n = 0; n < parsed.size(); n++) {
        if (!parsed[n] || size_t (std::abs (parsed[n])) > ndim)
          throw Exception ("axis ordering " + str (parsed[n]) + " out of range");

        for (size_t i = 0; i < n; i++)
          if (std::abs (parsed[i]) == std::abs (parsed[n]))
            throw Exception ("duplicate axis ordering (" + str (std::abs (parsed[n])) + ")");
      }

      return parsed;
    }



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

      auto i = H.keyval().find (flag);
      if (i == H.keyval().end())
        throw Exception ("missing \"" + flag + "\" specification for MRtrix image \"" + H.name() + "\"");
      const std::string path = i->second;
      H.keyval().erase (i);

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
        if (fname[0] != PATH_SEPARATOR[0]
#ifdef MRTRIX_WINDOWS
            && fname[0] != PATH_SEPARATOR[1]
#endif
           )
          fname = Path::join (Path::dirname (H.name()), fname);

      }

    }




  }
}
