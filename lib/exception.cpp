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

namespace MR
{


  void display_exception_cmdline (const Exception& E, int log_level)
  {
    for (size_t n = 0; n < E.description.size(); ++n) {
      switch (log_level) {
        case 1:
          error (E.description[n]);
          break;
        case 2:
          inform (E.description[n]);
          break;
        case 3:
          debug (E.description[n]);
          break;
      }
    }
  }

  void cmdline_print (const std::string& msg)
  {
    if (App::log_level) std::cerr << App::NAME << ": " << msg << "\n";
  }

  void cmdline_error (const std::string& msg)
  {
    if (App::log_level) std::cerr << App::NAME << " [ERROR]: " << msg << "\n";
  }

  void cmdline_info (const std::string& msg)
  {
    if (App::log_level > 1) std::cerr << App::NAME << " [INFO]: " <<  msg << "\n";
  }

  void cmdline_debug (const std::string& msg)
  {
    if (App::log_level > 2) std::cerr << App::NAME << " [DEBUG]: " <<  msg << "\n";
  }



  void (*print) (const std::string& msg) = cmdline_print;
  void (*error) (const std::string& msg) = cmdline_error;
  void (*inform) (const std::string& msg) = cmdline_info;
  void (*debug) (const std::string& msg) = cmdline_debug;

  void (*Exception::display_func) (const Exception& E, int log_level) = display_exception_cmdline;

}

