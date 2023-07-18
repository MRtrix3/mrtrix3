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





  vector<std::string> split (const std::string& string, const char* delimiters, bool ignore_empty_fields, size_t num)
  {
    vector<std::string> V;
    if (!string.size())
      return V;
    std::string::size_type start = 0, end;
    try {
      if (ignore_empty_fields)
        start = string.find_first_not_of (delimiters);
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





  namespace {

    // from https://www.geeksforgeeks.org/wildcard-character-matching/

    inline bool __match (const char* first, const char* second)
    {
      // If we reach at the end of both strings, we are done
      if (*first == '\0' && *second == '\0')
        return true;

      // Make sure that the characters after '*' are present
      // in second string. This function assumes that the first
      // string will not contain two consecutive '*'
      if (*first == '*' && *(first+1) != '\0' && *second == '\0')
        return false;

      // If the first string contains '?', or current characters
      // of both strings match
      if (*first == '?' || *first == *second)
        return match(first+1, second+1);

      // If there is *, then there are two possibilities
      // a) We consider current character of second string
      // b) We ignore current character of second string.
      if (*first == '*')
        return match(first+1, second) || match(first, second+1);

      return false;
    }
  }



  bool match (const std::string& pattern, const std::string& text, bool ignore_case)
  {
    if (ignore_case)
      return __match (lowercase(pattern).c_str(), lowercase (text).c_str());
    else
      return __match (pattern.c_str(), text.c_str());
  }



}
