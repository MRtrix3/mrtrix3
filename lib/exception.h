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

#ifndef __mrtrix_exception_h__
#define __mrtrix_exception_h__

#include <cerrno>
#include <string>
#include <vector>

#include "types.h"

namespace MR
{
  namespace App 
  {
    extern int log_level;
    extern std::string NAME;
  }

  //! print primary output to stdout as-is. 
  /*! This function is intended for cases where the command's primary output is text, not
   * image data, etc. It is \e not designed for error or status reports: it
   * prints to stdout, whereas all reporting functions print to stderr. This is
   * to allow the output of the command to be used directly in text processing
   * pipeline or redirected to file. 
   * \note the use of stdout is normally reserved for piping data files (or at
   * least their filenames) between MRtrix commands. This function should
   * therefore never be used in commands that produce output images, as the two
   * different types of output may then interfere and cause unexpected issues. */
  extern void (*print) (const std::string& msg);



  extern void (*report_to_user_func) (const std::string& msg, int type);

  //! display error, warning, debug, etc. message to user at specified log level
  /*! types are: 0: error; 1: warning; 2: additional information; 3:
   * debugging information; anything else: none.
   *
   * log-levels are: 0: errors only; 1: warnings & errors (default);
   * 2: additional information, warnings & errors; 3 and above: all 
   * messages, including debugging information. */
  inline void report_to_user (const std::string& msg, int type, int log_level)
  {
    if (App::log_level >= log_level)
      report_to_user_func (msg, type);
  }


  inline void console (const std::string& msg, int log_level = 0) { report_to_user (msg, -1, log_level); }
  inline void error (const std::string& msg, int log_level_offset = 0) { report_to_user (msg, 0, log_level_offset); }
  inline void warning (const std::string& msg, int log_level_offset = 0) { report_to_user (msg, 1, 1+log_level_offset); }
  inline void inform (const std::string& msg, int log_level_offset = 0) { report_to_user (msg, 2, 2+log_level_offset); }
  inline void debug (const std::string& msg, int log_level_offset = 0) { report_to_user (msg, 3, 3+log_level_offset); }




  class Exception
  {
    public:
      Exception (const std::string& msg) {
        description.push_back (msg);
      }
      Exception (const Exception& previous_exception, const std::string& msg) :
        description (previous_exception.description) {
        description.push_back (msg);
      }

      void display (int log_level = 1) const {
        display_func (*this, log_level);
      }

      size_t num () const {
        return (description.size());
      }
      const std::string& operator[] (size_t n) const {
        return (description[n]);
      }

      static void (*display_func) (const Exception& E, int log_level);

      std::vector<std::string> description;
  };


  void display_exception_cmdline (const Exception& E, int log_level);
  void cmdline_print_func (const std::string& msg);
  void cmdline_report_to_user_func (const std::string& msg, int type);
}

#endif

