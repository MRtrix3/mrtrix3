/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include <cstdarg>

namespace MR {

/************************************************************************
 *                       Miscellaneous functions                        *
 ************************************************************************/

std::vector<default_type> parse_floats(const std::string &spec) {
  std::vector<default_type> V;
  if (spec.empty())
    throw Exception("floating-point sequence specifier is empty");
  std::string::size_type start = 0, end;
  default_type range_spec[3];
  int i = 0;
  try {
    do {
      end = spec.find_first_of(",:", start);
      std::string sub(spec.substr(start, end - start));
      range_spec[i] = (sub.empty() || sub == "nan") ? NAN : to<default_type>(sub);
      char last_char = end < spec.size() ? spec[end] : '\0';
      if (last_char == ':') {
        i++;
        if (i > 2)
          throw Exception("invalid number range in number sequence \"" + spec + "\"");
      } else {
        if (i) {
          if (i != 2)
            throw Exception("For floating-point ranges, must specify three numbers (start:step:end)");
          default_type first = range_spec[0], inc = range_spec[1], last = range_spec[2];
          if (!inc || (inc * (last - first) < 0.0) || !std::isfinite(first) || !std::isfinite(inc) ||
              !std::isfinite(last))
            throw Exception("Floating-point range does not form a finite set");
          default_type value = first;
          for (size_t mult = 0; (inc > 0.0f ? value < last - 0.5f * inc : value > last + 0.5f * inc); ++mult)
            V.push_back((value = first + (mult * inc)));
        } else {
          V.push_back(range_spec[0]);
        }
        i = 0;
      }
      start = end + 1;
    } while (end < spec.size());
  } catch (Exception &E) {
    throw Exception(E, "can't parse floating-point sequence specifier \"" + spec + "\"");
  }
  return (V);
}

std::vector<std::string>
split(const std::string &string, const char *delimiters, bool ignore_empty_fields, size_t num) {
  std::vector<std::string> V;
  if (string.empty())
    return V;
  std::string::size_type start = 0, end;
  try {
    if (ignore_empty_fields)
      start = string.find_first_not_of(delimiters);
    do {
      end = string.find_first_of(delimiters, start);
      V.push_back(string.substr(start, end - start));
      if (end >= string.size())
        break;
      start = ignore_empty_fields ? string.find_first_not_of(delimiters, end + 1) : end + 1;
      if (start > string.size())
        break;
      if (V.size() + 1 >= num) {
        V.push_back(string.substr(start));
        break;
      }
    } while (true);
  } catch (...) {
    throw Exception("can't split string \"" + string + "\"");
  }
  return V;
}

namespace {

// from https://www.geeksforgeeks.org/wildcard-character-matching/

inline bool __match(const char *first, const char *second) {
  // If we reach at the end of both strings, we are done
  if (*first == '\0' && *second == '\0')
    return true;

  // Make sure that the characters after '*' are present
  // in second string. This function assumes that the first
  // string will not contain two consecutive '*'
  if (*first == '*' && *(first + 1) != '\0' && *second == '\0')
    return false;

  // If the first string contains '?', or current characters
  // of both strings match
  if (*first == '?' || *first == *second)
    return match(first + 1, second + 1);

  // If there is *, then there are two possibilities
  // a) We consider current character of second string
  // b) We ignore current character of second string.
  if (*first == '*')
    return match(first + 1, second) || match(first, second + 1);

  return false;
}
} // namespace

bool match(const std::string &pattern, const std::string &text, bool ignore_case) {
  if (ignore_case)
    return __match(lowercase(pattern).c_str(), lowercase(text).c_str());
  else
    return __match(pattern.c_str(), text.c_str());
}

std::istream &getline(std::istream &stream, std::string &string) {
  std::getline(stream, string);
  if (!string.empty())
    if (string[string.size() - 1] == 015)
      string.resize(string.size() - 1);
  return stream;
}

std::string &add_line(std::string &original, const std::string &new_line) {
  return original.empty() ? (original = new_line) : (original += "\n" + new_line);
}

std::string shorten(const std::string &text, size_t longest, size_t prefix) {
  if (text.size() > longest)
    return (text.substr(0, prefix) + "..." + text.substr(text.size() - longest + prefix + 3));
  else
    return text;
}

std::string lowercase(const std::string &string) {
  std::string ret;
  ret.resize(string.size());
  transform(string.begin(), string.end(), ret.begin(), tolower);
  return ret;
}

std::string uppercase(const std::string &string) {
  std::string ret;
  ret.resize(string.size());
  transform(string.begin(), string.end(), ret.begin(), toupper);
  return ret;
}

std::string printf(const char *format, ...) {
  size_t len = 0;
  va_list list1, list2;
  va_start(list1, format);
  va_copy(list2, list1);
  len = vsnprintf(nullptr, 0, format, list1) + 1;
  va_end(list1);
  VLA(buf, char, len);
  vsnprintf(buf, len, format, list2);
  va_end(list2);
  return buf;
}

std::string strip(const std::string &string, const std::string &ws, bool left, bool right) {
  const std::string::size_type start = (left ? string.find_first_not_of(ws) : 0);
  if (start == std::string::npos)
    return "";
  const std::string::size_type end = (right ? string.find_last_not_of(ws) + 1 : std::string::npos);
  return string.substr(start, end - start);
}

std::string unquote(const std::string &string) {
  if (string.size() <= 2)
    return string;
  if (!(string.front() == '\"' && string.back() == '\"'))
    return string;
  std::string substring = string.substr(1, string.size() - 2);
  if (std::none_of(substring.begin(), substring.end(), [](const char &c) { return c == '\"'; }))
    return substring;
  return string;
}

void replace(std::string &string, char orig, char final) {
  for (auto &c : string)
    if (c == orig)
      c = final;
}

void replace(std::string &str, const std::string &from, const std::string &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

std::vector<std::string> split_lines(const std::string &string, bool ignore_empty_fields, size_t num) {
  return split(string, "\n", ignore_empty_fields, num);
}

size_t char_is_dash(const char *arg) {
  assert(arg != nullptr);
  if (arg[0] == '-')
    return 1;
  if (arg[0] == '\0' || arg[1] == '\0' || arg[2] == '\0')
    return 0;
  const unsigned char *uarg = reinterpret_cast<const unsigned char *>(arg);
  if (uarg[0] == 0xE2 && uarg[1] == 0x80 && (uarg[2] >= 0x90 && uarg[2] <= 0x95))
    return 3;
  if (uarg[0] == 0xEF) {
    if (uarg[1] == 0xB9 && (uarg[2] == 0x98 || uarg[2] == 0xA3))
      return 3;
    if (uarg[1] == 0xBC && uarg[2] == 0x8D)
      return 3;
  }
  return 0;
}

bool is_dash(std::string_view arg) {
  const size_t nbytes = char_is_dash(arg.data());
  return nbytes != 0 && nbytes == arg.size();
}

bool starts_with_dash(std::string_view arg) { return char_is_dash(arg.data()) != 0U; }

std::string_view without_leading_dash(std::string_view arg) {
  size_t nbytes = char_is_dash(arg.data());
  while (nbytes > 0) {
    arg.remove_prefix(nbytes);
    nbytes = char_is_dash(arg.data());
  }
  return arg;
}

std::string join(const std::vector<std::string> &V, const std::string &delimiter) {
  std::string ret;
  if (V.empty())
    return ret;
  ret = V[0];
  for (std::vector<std::string>::const_iterator i = V.begin() + 1; i != V.end(); ++i)
    ret += delimiter + *i;
  return ret;
}

std::string join(const char *const *null_terminated_array, const std::string &delimiter) {
  std::string ret;
  if (!null_terminated_array)
    return ret;
  ret = null_terminated_array[0];
  for (const char *const *p = null_terminated_array + 1; *p; ++p)
    ret += delimiter + *p;
  return ret;
}

} // namespace MR
