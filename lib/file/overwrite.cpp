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

#include <iostream>

#include "app.h"
#include "debug.h"
#include "file/overwrite.h"

namespace MR
{
  namespace File
  {

    char confirm_overwrite_cmdline_func (const std::string& filename, bool yestoall)
    {
      std::cerr << App::name() << ": overwrite '" << filename << 
        ( yestoall ? "' (Yes|yes to All|No) (y|a|N) ? " : "' (Yes|No) (y|N) ? " );
      std::string response;
      std::cin >> response;
      if (response.empty()) return 'n';
      return response[0];
    }


    char (*confirm_overwrite_func) (const std::string& filename, bool yestoall) = confirm_overwrite_cmdline_func;


  }
}



