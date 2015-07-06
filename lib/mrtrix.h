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

#define GUI_SPACING 5

#define MODIFIERS ( \
                    GDK_SHIFT_MASK | \
                    GDK_CONTROL_MASK | \
                    GDK_BUTTON1_MASK | \
                    GDK_BUTTON2_MASK | \
                    GDK_BUTTON3_MASK | \
                    GDK_BUTTON4_MASK | \
                    GDK_BUTTON5_MASK | \
                    GDK_SUPER_MASK | \
                    GDK_HYPER_MASK | \
                    GDK_META_MASK )


#ifndef MIN
#  define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#  define MAX(a,b) ((a)>(b)?(a):(b))
#endif


namespace std
{

  template <class T> inline ostream& operator<< (ostream& stream, const vector<T>& V)
  {
    stream << "[ ";
    for (size_t n = 0; n < V.size(); n++) 
      stream << V[n] << " ";
    stream << "]";
    return stream;
  }

  template <class T, std::size_t N> inline ostream& operator<< (ostream& stream, const array<T,N>& V)
  {
    stream << "[ ";
    for (size_t n = 0; n < N; n++) 
      stream << V[n] << " ";
    stream << "]";
    return stream;
  }

}


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



  template <class T> inline std::string str (const T& value, int precision = 0)
  {
    std::ostringstream stream;
    stream.precision (precision ? precision : std::numeric_limits<T>::max_digits10);
    stream << value;
    if (stream.fail())
      throw Exception ("error converting value to string");
    return stream.str();
  }


  inline std::string& add_line (std::string& original, const std::string& new_line)
  {
    return original.size() ? (original += "\n" + new_line) : ( original = new_line );
  }


  inline std::string shorten (const std::string& text, size_t longest = 40, size_t prefix = 10)
  {
    if (text.size() > longest)
      return (text.substr (0,prefix) + "..." + text.substr (text.size()-longest+prefix+3));
    else 
      return text;
  }


  inline std::string& lowercase (std::string& string)
  {
    for (std::string::iterator i = string.begin(); i != string.end(); ++i)
      *i = tolower (*i);
    return string;
  }

  inline std::string& uppercase (std::string& string)
  {
    for (std::string::iterator i = string.begin(); i != string.end(); ++i)
      *i = toupper (*i);
    return string;
  }

  inline std::string lowercase (const std::string& string)
  {
    std::string ret;
    ret.resize (string.size());
    transform (string.begin(), string.end(), ret.begin(), tolower);
    return ret;
  }

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
      if (lowercase (string) == "nan") 
        return std::numeric_limits<T>::quiet_NaN();
      else if (lowercase (string) == "-nan") 
        return -std::numeric_limits<T>::quiet_NaN();
      else if (lowercase (string) == "inf") 
        return std::numeric_limits<T>::infinity();
      else if (lowercase (string) == "-inf")
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


  std::vector<std::string> split (
    const std::string& string,
    const char* delimiters = " \t\n",
    bool ignore_empty_fields = false,
    size_t num = std::numeric_limits<size_t>::max());

  inline std::vector<std::string> split_lines (
      const std::string& string,
      bool ignore_empty_fields = true,
      size_t num = std::numeric_limits<size_t>::max()) {
    return split (string, "\n", ignore_empty_fields, num);
  }

  inline std::string join (const std::vector<std::string>& V, const std::string& delimiter)
  {
    std::string ret;
    if (V.empty())
      return ret;
    ret = V[0];
    for (std::vector<std::string>::const_iterator i = V.begin() +1; i != V.end(); ++i)
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

  std::vector<default_type> parse_floats (const std::string& spec);
  std::vector<int>   parse_ints (const std::string& spec, int last = std::numeric_limits<int>::max());

  inline int round (default_type x)
  {
    return int (x + (x > 0.0 ? 0.5 : -0.5));
  }




  template <class T> inline T maxvalue (const T& v0, const T& v1, const T& v2)
  {
    return v0 > v1 ? (v0 > v2 ? v0 : (v1 > v2 ? v1 : v2)) : (v1 > v2 ? v1 : (v0 > v2 ? v0 : v2));
  }

  template <class T> inline int maxindex (const T& v0, const T& v1, const T& v2)
  {
    return v0 > v1 ? (v0 > v2 ? 0 : (v1 > v2 ? 1 : 2)) : (v1 > v2 ? 1 : (v0 > v2 ? 0 : 2));
  }

  template <class T> inline T maxvalue (const T v[3])
  {
    return maxvalue (v[0], v[1], v[2]);
  }
  template <class T> inline int maxindex (const T v[3])
  {
    return maxindex (v[0], v[1], v[2]);
  }




  template <class T> inline constexpr T minvalue (const T& v0, const T& v1, const T& v2)
  {
    return v0 < v1 ? (v0 < v2 ? v0 : (v1 < v2 ? v1 : v2)) : (v1 < v2 ? v1 : (v0 < v2 ? v0 : v2));
  }

  template <class T> inline constexpr int minindex (const T& v0, const T& v1, const T& v2)
  {
    return v0 < v1 ? (v0 < v2 ? 0 : (v1 < v2 ? 1 : 2)) : (v1 < v2 ? 1 : (v0 < v2 ? 0 : 2));
  }

  template <class T> inline constexpr T minvalue (const T v[3])
  {
    return minvalue (v[0], v[1], v[2]);
  }
  template <class T> inline constexpr int minindex (const T v[3])
  {
    return minindex (v[0], v[1], v[2]);
  }




  template <class T> inline void set_all (std::vector<T>& V, const T& value)
  {
    for (size_t n = 0; n < V.size(); n++) 
      V[n] = value;
  }




  template <class T> inline bool get_next (std::vector<T>& pos, const std::vector<T>& limits)
  {
    size_t axis = 0;
    while (axis < limits.size()) {
      pos[axis]++;
      if (pos[axis] < limits[axis]) 
        return true;
      pos[axis] = 0;
      axis++;
    }
    return false;
  }





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

    if (stream.peek() == 'i')
      return cfloat (0.0f, real);

    stream >> imag;
    if (stream.fail()) 
      return cfloat (real, 0.0f);
    else if (stream.peek() != 'i')
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

    if (stream.peek() == 'i') 
      return cdouble (0.0, real);

    stream >> imag;
    if (stream.fail()) 
      return cdouble (real, 0.0);
    else if (stream.peek() != 'i')
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

