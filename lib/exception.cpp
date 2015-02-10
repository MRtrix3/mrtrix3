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

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "debug.h"

#ifdef MRTRIX_AS_R_LIBRARY
# include "wrap_r.h"
#endif


namespace MR
{

  void display_exception_cmdline (const Exception& E, int log_level)
  {
    if (App::log_level >= log_level) 
      for (size_t n = 0; n < E.description.size(); ++n) 
        report_to_user_func (E.description[n], log_level);
  }

  off_t __stderr_offset = 0;


  namespace {

    inline const char* console_prefix (int type) 
    { 
      switch (type) {
        case 0: return "[ERROR] ";
        case 1: return "[WARNING] ";
        case 2: return "[INFO] ";
        case 3: return "[DEBUG] ";
        default: return "";
      }
    }

    inline const char* console_color (int type = -1) 
    {
      if (!App::terminal_use_colour) return "";
      switch (type) {
        case -1: return "\033[0m";
        case 0: return "\033[01;31m";
        case 1: return "\033[00;31m";
        case 2: return "\033[03;32m";
        case 3: return "\033[03;34m";
        default: return "";
      }
    }

  }

  void cmdline_report_to_user_func (const std::string& msg, int type)
  {
    __print_stderr (printf ("%s: %s%s%s%s\n", App::NAME.c_str(), console_color(type), console_prefix (type), msg.c_str(), console_color()));
    __stderr_offset = 0;
    if (type == 1 && App::fail_on_warn)
      throw Exception ("terminating due to request to fail on warning");
  }



  void cmdline_print_func (const std::string& msg)
  {
#ifdef MRTRIX_AS_R_LIBRARY
    Rprintf (msg.c_str());
#else 
    std::cout << msg;
#endif
  }




  void (*print) (const std::string& msg) = cmdline_print_func;
  void (*report_to_user_func) (const std::string& msg, int type) = cmdline_report_to_user_func;
  void (*Exception::display_func) (const Exception& E, int log_level) = display_exception_cmdline;

}

