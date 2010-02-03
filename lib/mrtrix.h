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
#include <string>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <limits>


#include "types.h"
#include "exception.h"

#define MRTRIX_MAJOR_VERSION 0
#define MRTRIX_MINOR_VERSION 3
#define MRTRIX_MICRO_VERSION 3


/** Prints the file and line number. Useful for debugging purposes. */
#define TEST std::cerr << MR::get_application_name() << ": line " << __LINE__ \
  << " in " << __func__ << "() from file " << __FILE__ << "\n"


/** Prints a variable name and its value, followed by the file and line number. Useful for debugging purposes. */
#define VAR(variable) std::cerr << MR::get_application_name() << ": " << #variable << " = " << (variable) \
  << " (in " << __func__ << "() from " << __FILE__  << ": " << __LINE__ << ")\n"

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


namespace std {

  template <class T> inline ostream& operator<< (ostream& stream, const vector<T>& V)
  {
    stream << "[ ";
    for (size_t n = 0; n < V.size(); n++) stream << V[n] << " ";
    stream << "]";
    return (stream);
  }

}


namespace MR {

  const std::string& get_application_name ();

  extern const size_t mrtrix_major_version;
  extern const size_t mrtrix_minor_version;
  extern const size_t mrtrix_micro_version;

  namespace Image {
    typedef enum {
      Default,
      Magnitude,
      Real,
      Imaginary,
      Phase,
      RealImag
    } OutputType;
  }




  inline std::istream& getline (std::istream& stream, std::string& string) 
  {
    std::getline (stream, string);
    if (string.size() > 0) if (string[string.size()-1] == 015) string.resize (string.size()-1);
    return (stream);
  }



  template <class T> inline std::string str (const T& value)
  { 
    std::ostringstream stream; 
    try { stream << value; }
    catch (...) { throw Exception ("error converting value to string"); }
    return (stream.str());
  }




  template <class T> inline T to (const std::string& string)
  {
    std::istringstream stream (string);
    T value;
    try { stream >> value; }
    catch (...) { throw Exception ("error converting string \"" + string + "\""); }
    return (value);
  }


  inline std::string printf (const char* format, ...)
  {
    va_list list;
    va_start (list, format);
    size_t len = vsnprintf (NULL, 0, format, list) + 1;
    va_end (list);
    char buf[len];
    va_start (list, format);
    vsnprintf (buf, len, format, list);
    va_end (list);
    return (buf);
  }


  inline std::string strip (const std::string& string, const char* ws = " \t\n", bool left = true, bool right = true)
  {
    std::string::size_type start = ( left ? string.find_first_not_of (ws) : 0 );
    if (start == std::string::npos) return ("");
    std::string::size_type end   = ( right ? string.find_last_not_of (ws) + 1 : std::string::npos );
    return (string.substr (start, end - start));
  }



  inline void  replace (std::string& string, char orig, char final)
  {
    for (std::string::iterator i = string.begin(); i != string.end(); ++i)
      if (*i == orig) *i = final;
  }


  inline std::string& lowercase (std::string& string)
  {
    for (std::string::iterator i = string.begin(); i != string.end(); ++i) *i = tolower (*i);
    return (string);
  }

  inline std::string& uppercase (std::string& string)
  {
    for (std::string::iterator i = string.begin(); i != string.end(); ++i) *i = toupper (*i);
    return (string);
  }

  inline std::string lowercase (const std::string& string)
  {
    std::string ret; ret.resize (string.size());
    transform (string.begin(), string.end(), ret.begin(), tolower);
    return (ret);
  }

  inline std::string uppercase (const std::string& string)
  {
    std::string ret; ret.resize (string.size());
    transform (string.begin(), string.end(), ret.begin(), toupper);
    return (ret);
  }

  std::vector<std::string> split (const std::string& string, const char* delimiters = " \t\n", bool ignore_empty_fields = false, size_t num = SIZE_MAX); 

  inline std::string join (std::vector<std::string>& V, const std::string& delimiter)
  {
    std::string ret;
    if (V.empty()) return (ret);
    ret = V[0];
    for (std::vector<std::string>::iterator i = V.begin()+1; i != V.end(); ++i)
      ret += delimiter + *i;
    return (ret);
  }

  std::vector<float> parse_floats (const std::string& spec);
  std::vector<int>   parse_ints (const std::string& spec, int last = std::numeric_limits<int>::max());

  inline int round (float x) { return (int (x + (x > 0.0 ? 0.5 : -0.5))); }




  template <class T> inline T maxvalue (const T& v0, const T& v1, const T& v2)
  {
    return (v0 > v1 ? ( v0 > v2 ? v0 : ( v1 > v2 ? v1 : v2 ) ) : ( v1 > v2 ? v1 : ( v0 > v2 ? v0 : v2 ))); 
  }

  template <class T> inline int maxindex (const T& v0, const T& v1, const T& v2)
  {
    return (v0 > v1 ? ( v0 > v2 ? 0 : ( v1 > v2 ? 1 : 2 ) ) : ( v1 > v2 ? 1 : ( v0 > v2 ? 0 : 2 ))); 
  }

  template <class T> inline T maxvalue (const T v[3]) { return (maxvalue (v[0], v[1], v[2])); }
  template <class T> inline int maxindex (const T v[3]) { return (maxindex (v[0], v[1], v[2])); }




  template <class T> inline T minvalue (const T& v0, const T& v1, const T& v2)
  {
    return (v0 < v1 ? ( v0 < v2 ? v0 : ( v1 < v2 ? v1 : v2 ) ) : ( v1 < v2 ? v1 : ( v0 < v2 ? v0 : v2 ))); 
  }

  template <class T> inline int minindex (const T& v0, const T& v1, const T& v2)
  {
    return (v0 < v1 ? ( v0 < v2 ? 0 : ( v1 < v2 ? 1 : 2 ) ) : ( v1 < v2 ? 1 : ( v0 < v2 ? 0 : 2 ))); 
  }

  template <class T> inline T minvalue (const T v[3]) { return (minvalue (v[0], v[1], v[2])); }
  template <class T> inline int minindex (const T v[3]) { return (minindex (v[0], v[1], v[2])); }




  template <class T> inline void set_all (std::vector<T>& V, const T& value)
  {
    for (size_t n = 0; n < V.size(); n++) V[n] = value;
  }




  template <class T> inline bool get_next (std::vector<T>& pos, const std::vector<T>& limits)
  {
    size_t axis = 0;
    while (axis < limits.size()) {
      pos[axis]++;
      if (pos[axis] < limits[axis]) return (true);
      pos[axis] = 0;
      axis++;
    }
    return (false);
  }




}

#endif

