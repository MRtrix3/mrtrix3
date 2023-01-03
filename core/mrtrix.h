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

#ifndef __mrtrix_h__
#define __mrtrix_h__

#include <climits>
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <array>
#include <string>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <iomanip>


#include "types.h"
#include "exception.h"


namespace MR
{

  //! read a line from the stream
  /*! a replacement for the standard getline() function that also discards
   * carriage returns if found at the end of the line. */
  inline std::istream& getline (std::istream& stream, std::string& string)
  {
    std::getline (stream, string);
    if (string.size() > 0)
      if (string[string.size()-1] == 015)
        string.resize (string.size()-1);
    return stream;
  }



  template <typename X, typename ReturnType = int>
    struct max_digits { NOMEMALIGN
      static constexpr int value () { return 0; }
    };

  template <typename X>
    struct max_digits<X, typename std::enable_if<std::is_fundamental<X>::value, int>::type> { NOMEMALIGN
      static constexpr int value () { return std::numeric_limits<X>::max_digits10; }
    };

  template <typename X>
    struct max_digits<X, typename std::enable_if<std::is_fundamental<typename X::Scalar>::value, int>::type> { NOMEMALIGN
      static constexpr int value () { return std::numeric_limits<typename X::Scalar>::max_digits10; }
    };

  template <typename X>
    struct max_digits<X, typename std::enable_if<std::is_fundamental<typename X::value_type>::value && !std::is_fundamental<typename X::Scalar>::value, int>::type> { NOMEMALIGN
      static constexpr int value () { return std::numeric_limits<typename X::value_type>::max_digits10; }
    };



  //! add a line to a string, taking care of inserting a newline if needed
  inline std::string& add_line (std::string& original, const std::string& new_line)
  {
    return original.size() ? (original += "\n" + new_line) : ( original = new_line );
  }


  //! convert a long string to 'beginningofstring...endofstring' for display
  inline std::string shorten (const std::string& text, size_t longest = 40, size_t prefix = 10)
  {
    if (text.size() > longest)
      return (text.substr (0,prefix) + "..." + text.substr (text.size()-longest+prefix+3));
    else
      return text;
  }


  //! return lowercase version of string
  inline std::string lowercase (const std::string& string)
  {
    std::string ret;
    ret.resize (string.size());
    transform (string.begin(), string.end(), ret.begin(), tolower);
    return ret;
  }

  //! return uppercase version of string
  inline std::string uppercase (const std::string& string)
  {
    std::string ret;
    ret.resize (string.size());
    transform (string.begin(), string.end(), ret.begin(), toupper);
    return ret;
  }


  inline std::string printf (const char* format, ...)
  {
    size_t len = 0;
    va_list list1, list2;
    va_start (list1, format);
    va_copy (list2, list1);
    len = vsnprintf (nullptr, 0, format, list1) + 1;
    va_end (list1);
    VLA(buf, char, len);
    vsnprintf (buf, len, format, list2);
    va_end (list2);
    return buf;
  }


  inline std::string strip (const std::string& string, const std::string& ws = {" \0\t\r\n", 5}, bool left = true, bool right = true)
  {
    std::string::size_type start = (left ? string.find_first_not_of (ws) : 0);
    if (start == std::string::npos)
      return "";
    std::string::size_type end = (right ? string.find_last_not_of (ws) + 1 : std::string::npos);
    return string.substr (start, end - start);
  }


  //! Remove quotation marks only if surrounding entire string
  inline std::string unquote (const std::string& string)
  {
    if (string.size() <= 2)
      return string;
    if (!(string.front() == '\"' && string.back() == '\"'))
      return string;
    const std::string substring = string.substr(1, string.size()-2);
    if (std::none_of (substring.begin(), substring.end(), [] (const char& c) { return c == '\"'; }))
      return substring;
    return string;
  }



  inline void replace (std::string& string, char orig, char final)
  {
    for (auto& c: string)
      if (c == orig) c = final;
  }

  inline void replace (std::string& str, const std::string& from, const std::string& to)
  {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find (from, start_pos)) != std::string::npos) {
      str.replace (start_pos, from.length(), to);
      start_pos += to.length();
    }
  }


  vector<std::string> split (
    const std::string& string,
    const char* delimiters = " \t\n",
    bool ignore_empty_fields = false,
    size_t num = std::numeric_limits<size_t>::max());

  inline vector<std::string> split_lines (
      const std::string& string,
      bool ignore_empty_fields = true,
      size_t num = std::numeric_limits<size_t>::max())
  {
    return split (string, "\n", ignore_empty_fields, num);
  }




  /*
  inline int round (default_type x)
  {
    return int (x + (x > 0.0 ? 0.5 : -0.5));
  }
*/


  bool match (const std::string& pattern, const std::string& text, bool ignore_case = false);



  //! match a dash or any Unicode character that looks like one
  /*! \note This returns the number of bytes taken up by the matched UTF8
   * character, zero if no match. */
  inline size_t char_is_dash (const char* arg)
  {
    assert (arg != nullptr);
    if (arg[0] == '-')
     return 1;
    if (arg[0] == '\0' || arg[1] == '\0' || arg[2] == '\0')
      return 0;
    const unsigned char* uarg = reinterpret_cast<const unsigned char*> (arg);
    if (uarg[0] == 0xE2 && uarg[1] == 0x80 && ( uarg[2] >= 0x90 && uarg[2] <= 0x95 ))
      return 3;
    if (uarg[0] == 0xEF) {
     if (uarg[1] == 0xB9 && ( uarg[2] == 0x98 || uarg[2] == 0xA3))
       return 3;
     if (uarg[1] == 0xBC && uarg[2] == 0x8D)
       return 3;
    }
    return 0;
  }

  //! match whole string to a dash or any Unicode character that looks like one
  inline size_t is_dash (const std::string& arg)
  {
    size_t nbytes = char_is_dash (arg.c_str());
    return nbytes != 0 && nbytes == arg.size();
  }


  //! match current character to a dash or any Unicode character that looks like one
  /*! \note If a match is found, this also advances the \a arg pointer to the next
   * character in the string, which could be one or several bytes along depending on
   * the width of the UTF8 character identified. */
  inline bool consume_dash (const char*& arg)
  {
    size_t nbytes = char_is_dash (arg);
    arg += nbytes;
    return nbytes != 0;
  }





  template <class T> inline std::string str (const T& value, int precision = 0)
  {
    std::ostringstream stream;
    if (precision)
      stream.precision (precision);
    else if (max_digits<T>::value())
      stream.precision (max_digits<T>::value());
    stream << value;
    if (stream.fail())
      throw Exception (std::string("error converting type \"") + typeid(T).name() + "\" value to string");
    return stream.str();
  }

  template <class T> inline T to (const std::string& string)
  {
    const std::string stripped (strip (string));
    std::istringstream stream (stripped);
    T value;
    stream >> value;
    if (stream.fail()) {
      if (std::is_floating_point<T>::value) {
        const std::string lstring = lowercase (stripped);
        if (lstring == "nan")
          return std::numeric_limits<T>::quiet_NaN();
        else if (lstring == "-nan")
          return -std::numeric_limits<T>::quiet_NaN();
        else if (lstring == "inf")
          return std::numeric_limits<T>::infinity();
        else if (lstring == "-inf")
          return -std::numeric_limits<T>::infinity();
      }
      throw Exception ("error converting string \"" + string + "\" to type \"" + typeid(T).name() + "\"");
    } else if (!stream.eof()) {
      throw Exception ("incomplete use of string \"" + string + "\" in conversion to type \"" + typeid(T).name() + "\"");
    }
    return value;
  }

  template <> inline bool to<bool> (const std::string& string)
  {
    std::string value = lowercase (strip (string));
    if (value == "true" || value == "yes")
      return true;
    if (value == "false" || value == "no")
      return false;
    return to<int> (string);
  }

  template <> inline std::string str<cfloat> (const cfloat& value, int precision)
  {
    std::ostringstream stream;
    if (precision > 0)
      stream.precision (precision);
    stream << value.real();
    if (value.imag())
      stream << std::showpos << value.imag() << "i";
    if (stream.fail())
      throw Exception ("error converting complex float value to string");
    return stream.str();
  }

  template <> inline cfloat to<cfloat> (const std::string& string)
  {
    if (!string.size())
      throw Exception ("cannot convert empty string to complex float");

    const std::string stripped = strip (string);
    vector<cfloat> candidates;
    for (ssize_t i = -1; i <= ssize_t(stripped.size()); ++i) {
      std::string first, second;
      if (i == -1) {
        first = "0";
        second = stripped;
      } else if (i == ssize_t(stripped.size())) {
        first = stripped;
        second = "0i";
      } else {
        if (!(stripped[i] == '+' || stripped[i] == '-'))
          continue;
        first = stripped.substr(0, i);
        second = stripped.substr(stripped[i] == '-' ? i : i+1);
      }
      if (!(second.back() == 'i' || second.back() == 'j'))
        continue;
      second.pop_back();
      if (second.empty() || second == "-" || second == "+")
        second.push_back ('1');
      try {
        candidates.push_back (cfloat (to<float> (first), to<float> (second)));
      } catch (Exception&) { }
    }

    if (candidates.empty())
      throw Exception ("error converting string \"" + string + "\" to complex float (no valid conversion)");

    for (size_t i = 1; i != candidates.size(); ++i) {
      if (!(candidates[i].real() == candidates[0].real() ||
            (std::isnan (candidates[i].real()) && std::isnan (candidates[0].real()))))
        throw Exception ("error converting string \"" + string + "\" to complex float (ambiguity in real component)");
      if (!(candidates[i].imag() == candidates[0].imag() ||
            (std::isnan (candidates[i].imag()) && std::isnan (candidates[0].imag()))))
        throw Exception ("error converting string \"" + string + "\" to complex float (ambiguity in imaginary component)");
    }
    return candidates[0];
  }

  template <> inline std::string str<cdouble> (const cdouble& value, int precision)
  {
    std::ostringstream stream;
    if (precision > 0)
      stream.precision (precision);
    stream << value.real();
    if (value.imag())
      stream << std::showpos << value.imag() << "i";
    if (stream.fail())
      throw Exception ("error converting complex double value to string");
    return stream.str();
  }

  template <> inline cdouble to<cdouble> (const std::string& string)
  {
    if (!string.size())
      throw Exception ("cannot convert empty string to complex double");

    const std::string stripped = strip (string);
    vector<cdouble> candidates;
    for (ssize_t i = -1; i <= ssize_t(stripped.size()); ++i) {
      std::string first, second;
      if (i == -1) {
        first = "0";
        second = stripped;
      } else if (i == ssize_t(stripped.size())) {
        first = stripped;
        second = "0i";
      } else {
        if (!(stripped[i] == '+' || stripped[i] == '-'))
          continue;
        first = stripped.substr(0, i);
        second = stripped.substr(stripped[i] == '-' ? i : i+1);
      }
      if (!(second.back() == 'i' || second.back() == 'j'))
        continue;
      second.pop_back();
      if (second.empty() || second == "-" || second == "+")
        second.push_back ('1');
      try {
        candidates.push_back (cdouble (to<double> (first), to<double> (second)));
      } catch (Exception&) { }
    }

    if (candidates.empty())
      throw Exception ("error converting string \"" + string + "\" to complex double (no valid conversion)");

    for (size_t i = 1; i != candidates.size(); ++i) {
      if (!(candidates[i].real() == candidates[0].real() ||
            (std::isnan (candidates[i].real()) && std::isnan (candidates[0].real()))))
        throw Exception ("error converting string \"" + string + "\" to complex double (ambiguity in real component)");
      if (!(candidates[i].imag() == candidates[0].imag() ||
            (std::isnan (candidates[i].imag()) && std::isnan (candidates[0].imag()))))
        throw Exception ("error converting string \"" + string + "\" to complex double (ambiguity in imaginary component)");
    }
    return candidates[0];
  }







  vector<default_type> parse_floats (const std::string& spec);



  template <typename IntType>
  vector<IntType> parse_ints (const std::string& spec, const IntType last = std::numeric_limits<IntType>::max())
  {
    typedef typename std::make_signed<IntType>::type SignedIntType;
    if (!spec.size()) throw Exception ("integer sequence specifier is empty");

    auto to_unsigned = [&] (const SignedIntType value)
    {
      if (std::is_unsigned<IntType>::value && value < 0)
        throw Exception ("Impermissible negative value present in sequence \"" + spec + "\"");
      return IntType(value);
    };

    vector<IntType> V;
    std::string::size_type start = 0, end;
    std::array<SignedIntType, 3> num;
    size_t i = 0;
    try {
      do {
        start = spec.find_first_not_of (" \t", start);
        if (start == std::string::npos)
          break;
        end = spec.find_first_of (" \t,:", start);
        std::string token (strip (spec.substr (start, end-start)));
        if (lowercase (token) == "end") {
          if (last == std::numeric_limits<IntType>::max())
            throw Exception ("value of \"end\" is not known in number sequence \"" + spec + "\"");
          num[i] = SignedIntType(last);
        }
        else num[i] = to<SignedIntType> (spec.substr (start, end-start));

        end = spec.find_first_not_of (" \t", end);
        char last_char = end < spec.size() ? spec[end] : '\0';
        if (last_char == ':') {
          ++i;
          ++end;
          if (i > 2) throw Exception ("invalid number range in number sequence \"" + spec + "\"");
        } else {
          if (i) {
            SignedIntType inc, last;
            if (i == 2) {
              inc = num[1];
              last = num[2];
            }
            else {
              inc = 1;
              last = num[1];
            }
            if (inc * (last - num[0]) < 0)
              inc = -inc;
            for (; (inc > 0 ? num[0] <= last : num[0] >= last) ; num[0] += inc)
              V.push_back (to_unsigned(num[0]));
          }
          else V.push_back (to_unsigned(num[0]));
          i = 0;
        }

        start = end;
        if (last_char == ',')
          ++start;
      }
      while (end < spec.size());
    }
    catch (Exception& E) {
      throw Exception (E, "can't parse integer sequence specifier \"" + spec + "\"");
    }

    return (V);
  }








  inline std::string join (const vector<std::string>& V, const std::string& delimiter)
  {
    std::string ret;
    if (V.empty())
      return ret;
    ret = V[0];
    for (vector<std::string>::const_iterator i = V.begin() +1; i != V.end(); ++i)
      ret += delimiter + *i;
    return ret;
  }

  template <typename T>
  inline std::string join (const vector<T>& V, const std::string& delimiter)
  {
    std::string ret;
    if (V.empty())
      return ret;
    ret = str(V[0]);
    for (typename vector<T>::const_iterator i = V.begin() +1; i != V.end(); ++i)
      ret += delimiter + str(*i);
    return ret;
  }

  inline std::string join (const char* const* null_terminated_array, const std::string& delimiter)
  {
    std::string ret;
    if (!null_terminated_array)
      return ret;
    ret = null_terminated_array[0];
    for (const char* const* p = null_terminated_array+1; *p; ++p)
      ret += delimiter + *p;
    return ret;
  }



}

#endif

