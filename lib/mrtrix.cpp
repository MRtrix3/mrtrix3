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

#include <gsl/gsl_version.h>

#include "mrtrix.h"
#include "image/object.h"

namespace MR {

  /************************************************************************
   *                     MRtrix version information                      *
   ************************************************************************/

  const uint mrtrix_major_version = MRTRIX_MAJOR_VERSION;
  const uint mrtrix_minor_version = MRTRIX_MINOR_VERSION;
  const uint mrtrix_micro_version = MRTRIX_MICRO_VERSION;


  /************************************************************************
   *                       Miscellaneous functions                        *
   ************************************************************************/

  std::vector<float> parse_floats (const std::string& spec)
  {
    std::vector<float> V;
    try {
      if (!spec.size()) throw (0);
      std::string::size_type start = 0, end;
      do {
        end = spec.find_first_of (',', start);
        std::string sub (spec.substr (start, end-start));
        float num = sub == "nan" ? NAN : to<float> (spec.substr (start, end-start));
        V.push_back (num);
        start = end+1;
      } while (end < spec.size());
    }
    catch (...) { throw Exception ("can't parse floating-point sequence specifier \"" + spec + "\""); }
    return (V);
  }




  std::vector<int> parse_ints (const std::string& spec, int last)
  {
    std::vector<int> V;
    try {
      if (!spec.size()) throw (0);
      std::string::size_type start = 0, end;
      int num[3];
      int i = 0;
      do {
        end = spec.find_first_of (",:", start);
        std::string str (strip (spec.substr (start, end-start)));
        lowercase (str);
        if (str == "end") {
          if (last == INT_MAX) throw Exception ("value of \"end\" is not known in number sequence \"" + spec + "\"");
          num[i] = last;
        }
        else num[i] = to<int> (spec.substr (start, end-start));

        char last_char = end < spec.size() ? spec[end] : '\0';
        if (last_char == ':') { i++; if (i > 2) throw Exception ("invalid number range in number sequence \"" + spec + "\""); }
        else {
          if (i) {
            int inc, last;
            if (i == 2) { inc = num[1]; last = num[2]; }
            else { inc = 1; last = num[1]; }
            if (inc * (last - num[0]) < 0) inc = -inc;
            for (; ( inc > 0 ? num[0] <= last : num[0] >= last ) ; num[0] += inc) V.push_back (num[0]);
          }
          else V.push_back (num[0]);
          i = 0;
        }

        start = end+1;
      } while (end < spec.size());
    }
    catch (...) { throw Exception ("can't parse integer sequence specifier \"" + spec + "\""); }
    return (V);
  }







  std::vector<std::string> split (const std::string& string, const char* delimiters, bool ignore_empty_fields)
  {
    std::vector<std::string> V;
    std::string::size_type start = 0, end;
    try {
      do {
        end = string.find_first_of (delimiters, start);
        V.push_back (string.substr (start, end-start));
        start = ignore_empty_fields ? string.find_first_not_of (delimiters, end+1) : end+1;
      } while (end < std::string::npos);
    }
    catch (...)  { throw Exception ("can't split string \"" + string + "\""); }
    return (V);
  }



}
