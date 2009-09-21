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
#include "types.h"

namespace MR {

  extern void (*print) (const std::string& msg);
  extern void (*error) (const std::string& msg);
  extern void (*info)  (const std::string& msg);
  extern void (*debug) (const std::string& msg);

  class Exception {
    public:
      Exception (const std::string& msg, int log_level = 1) : 
        description (msg),
        level (log_level) { display(); }

      const std::string description;
      const int level;

      void  display () const {
        if (level + level_offset < 2) error (description);
        else if (level + level_offset == 2) info (description);
        else debug (description);
      }

      class Lower {
        public:
          Lower (int amount = 1)  { level_offset = amount; }
          ~Lower () { level_offset = 0; }
          friend class Exception;
      };

    private:
      static int level_offset;
  };

}

#endif

