/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "mrtrix.h"

namespace MR
{


  /************************************************************************
   *                       Miscellaneous functions                        *
   ************************************************************************/

  vector<default_type> parse_floats (const std::string& spec)
  {
    vector<default_type> V;
    if (!spec.size()) throw Exception ("floating-point sequence specifier is empty");
    std::string::size_type start = 0, end;
    default_type range_spec[3];
    int i = 0;
    try {
      do {
        end = spec.find_first_of (",:", start);
        std::string sub (spec.substr (start, end-start));
        range_spec[i] = (sub.empty() || sub == "nan") ? NAN : to<default_type> (sub);
        char last_char = end < spec.size() ? spec[end] : '\0';
        if (last_char == ':') {
          i++;
          if (i > 2) throw Exception ("invalid number range in number sequence \"" + spec + "\"");
        } else {
          if (i) {
            if (i != 2)
              throw Exception ("For floating-point ranges, must specify three numbers (start:step:end)");
            default_type first = range_spec[0], inc = range_spec[1], last = range_spec[2];
            if (!inc || (inc * (last-first) < 0.0) || !std::isfinite(first) || !std::isfinite(inc) || !std::isfinite(last))
              throw Exception ("Floating-point range does not form a finite set");
            default_type value = first;
            for (size_t mult = 0; (inc>0.0f ? value < last - 0.5f*inc : value > last + 0.5f*inc); ++mult)
              V.push_back ((value = first + (mult * inc)));
          } else {
            V.push_back (range_spec[0]);
          }
          i = 0;
        }
        start = end+1;
      } while (end < spec.size());
    }
    catch (Exception& E) {
      throw Exception (E, "can't parse floating-point sequence specifier \"" + spec + "\"");
    }
    return (V);
  }




  vector<int> parse_ints (const std::string& spec, int last)
  {
    vector<int> V;
    if (!spec.size()) throw Exception ("integer sequence specifier is empty");
    std::string::size_type start = 0, end;
    int num[3];
    int i = 0;
    try {
      do {
        end = spec.find_first_of (",:", start);
        std::string token (strip (spec.substr (start, end-start)));
        lowercase (token);
        if (token == "end") {
          if (last == std::numeric_limits<int>::max())
            throw Exception ("value of \"end\" is not known in number sequence \"" + spec + "\"");
          num[i] = last;
        }
        else num[i] = to<int> (spec.substr (start, end-start));

        char last_char = end < spec.size() ? spec[end] : '\0';
        if (last_char == ':') {
          i++;
          if (i > 2) throw Exception ("invalid number range in number sequence \"" + spec + "\"");
        }
        else {
          if (i) {
            int inc, last;
            if (i == 2) {
              inc = num[1];
              last = num[2];
            }
            else {
              inc = 1;
              last = num[1];
            }
            if (inc * (last - num[0]) < 0) inc = -inc;
            for (; (inc > 0 ? num[0] <= last : num[0] >= last) ; num[0] += inc) V.push_back (num[0]);
          }
          else V.push_back (num[0]);
          i = 0;
        }

        start = end+1;
      }
      while (end < spec.size());
    }
    catch (Exception& E) {
      throw Exception (E, "can't parse integer sequence specifier \"" + spec + "\"");
    }

    return (V);
  }







  vector<std::string> split (const std::string& string, const char* delimiters, bool ignore_empty_fields, size_t num)
  {
    vector<std::string> V;
    std::string::size_type start = 0, end;
    try {
      do {
        end = string.find_first_of (delimiters, start);
        V.push_back (string.substr (start, end-start));
        if (end >= string.size()) break;
        start = ignore_empty_fields ? string.find_first_not_of (delimiters, end+1) : end+1;
        if (start > string.size()) break;
        if (V.size()+1 >= num) {
          V.push_back (string.substr (start));
          break;
        }
      } while (true);
    }
    catch (...)  {
      throw Exception ("can't split string \"" + string + "\"");
    }
    return V;
  }



}
