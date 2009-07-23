/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 05/07/09.

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

#include "file/confirm.h"
#include "mrtrix.h"

namespace MR {
  namespace File {

    bool (*confirm) (const std::string& message) = NULL;

    bool confirm_func_cmdline (const std::string& message) {
      print (message + " ");
      std::string response;
      getline (std::cin, response);
      return (lowercase (response).compare (0, response.size(), "yes", response.size()) == 0);
    }

  }
}

