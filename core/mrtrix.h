/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
#include <vector>
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



  template <class T> inline std::string str (const T& value, int precision = 0)
  {
    std::ostringstream stream;
    if (precision)
        stream.precision (precision);
    else if (max_digits<T>::value())
        stream.precision (max_digits<T>::value());
    stream << value;
    if (stream.fail())
      throw Exception ("error converting value to string");
    return stream.str();
  }



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

  template <class T> inline T to (const std::string& string)
  {
    std::istringstream stream (string);
    T value;
    stream >> value;
    if (stream.fail()) {
      const std::string lstring = lowercase (string);
      if (lstring == "nan")
        return std::numeric_limits<T>::quiet_NaN();
      else if (lstring == "-nan")
        return -std::numeric_limits<T>::quiet_NaN();
      else if (lstring == "inf")
        return std::numeric_limits<T>::infinity();
      else if (lstring == "-inf")
        return -std::numeric_limits<T>::infinity();
      throw Exception ("error converting string \"" + string + "\"");
    }
    return value;
  }


  inline std::string printf (const char* format, ...)
  {
    va_list list;
    va_start (list, format);
    size_t len = vsnprintf (NULL, 0, format, list) + 1;
    va_end (list);
    VLA(buf, char, len);
    va_start (list, format);
    vsnprintf (buf, len, format, list);
    va_end (list);
    return buf;
  }


  inline std::string strip (const std::string& string, const char* ws = " \t\n", bool left = true, bool right = true)
  {
    std::string::size_type start = (left ? string.find_first_not_of (ws) : 0);
    if (start == std::string::npos)
      return "";
    std::string::size_type end = (right ? string.find_last_not_of (ws) + 1 : std::string::npos);
    return string.substr (start, end - start);
  }



  inline void  replace (std::string& string, char orig, char final)
  {
    for (std::string::iterator i = string.begin(); i != string.end(); ++i)
      if (*i == orig) *i = final;
  }

  inline void  replace (std::string& str, const std::string& from, const std::string& to)
  {
    if (from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
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
      size_t num = std::numeric_limits<size_t>::max()) {
    return split (string, "\n", ignore_empty_fields, num);
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

  vector<default_type> parse_floats (const std::string& spec);
  vector<int>   parse_ints (const std::string& spec, int last = std::numeric_limits<int>::max());

  /*
  inline int round (default_type x)
  {
    return int (x + (x > 0.0 ? 0.5 : -0.5));
  }
*/


/**********************************************************************
   specialisations to convert complex numbers to/from text:
**********************************************************************/


  template <> inline std::string str<cfloat> (const cfloat& value, int precision)
  {
    std::ostringstream stream;
    if (precision > 0)
      stream.precision (precision);
    stream << value.real();
    if (value.imag())
      stream << std::showpos << value.imag() << "i";
    if (stream.fail())
      throw Exception ("error converting value to string");
    return stream.str();
  }


  template <> inline cfloat to<cfloat> (const std::string& string)
  {
    std::istringstream stream (string);
    float real, imag;
    stream >> real;
    if (stream.fail())
      throw Exception ("error converting string \"" + string + "\"");
    if (stream.eof())
      return cfloat (real, 0.0f);

    if (stream.peek() == 'i' || stream.peek() == 'j')
      return cfloat (0.0f, real);

    stream >> imag;
    if (stream.fail())
      return cfloat (real, 0.0f);
    else if (stream.peek() != 'i' && stream.peek() != 'j')
      throw Exception ("error converting string \"" + string + "\"");
    return cfloat (real, imag);
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
      throw Exception ("error converting value to string");
    return stream.str();
  }


  template <> inline cdouble to<cdouble> (const std::string& string)
  {
    std::istringstream stream (string);
    double real, imag;
    stream >> real;
    if (stream.fail())
      throw Exception ("error converting string \"" + string + "\"");
    if (stream.eof())
      return cdouble (real, 0.0);

    if (stream.peek() == 'i' || stream.peek() == 'j')
      return cdouble (0.0, real);

    stream >> imag;
    if (stream.fail())
      return cdouble (real, 0.0);
    else if (stream.peek() != 'i' && stream.peek() != 'j')
      throw Exception ("error converting string \"" + string + "\"");
    return cdouble (real, imag);
  }



  template <> inline bool to<bool> (const std::string& string)
  {
    std::string value = lowercase (string);
    if (value == "true" || value == "yes")
      return true;
    if (value == "false" || value == "no")
      return false;
    return to<int> (value);
  }


}

#endif

