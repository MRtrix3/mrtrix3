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

#ifndef __file_overwrite_h__
#define __file_overwrite_h__

#include <string>

#include "file/path.h"

namespace MR
{
  namespace File
  {

    extern char (*confirm_overwrite_func) (const std::string& filename, bool yestoall);

    char confirm_overwrite_cmdline_func (const std::string& filename, bool yestoall);



    class ConfirmOverwrite {
      public:
        ConfirmOverwrite () : yestoall (false) { };

        void operator() (const std::string& filename) {
          if (yestoall) return;
          if (Path::exists (filename)) {
            char response = confirm_overwrite_func (filename, true);
            if (response == 'y') return;
            if (response == 'a') {
              yestoall = true;
              return;
            }
            throw Exception ("file overwrite cancelled by user");
          }
        }

        static void single_file (const std::string& filename)
        {
          if (Path::exists (filename))
            if (confirm_overwrite_func (filename, false) != 'y')
              throw Exception ("file overwrite cancelled by user");
        }

      protected:
        bool yestoall;
    };


  }
}

#endif


