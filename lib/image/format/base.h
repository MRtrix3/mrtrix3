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

#ifndef __image_format_base_h__
#define __image_format_base_h__

#include "mrtrix.h"

namespace MR {
  namespace Image {

    class Mapper;
    class Header;

    namespace Format {

      extern bool print_all;
      extern const char* known_extensions[];

      class Base {
        public:
          Base (const char* desc) : description (desc) { }
          virtual ~Base() { }

          const char*        description;

          virtual bool        read (Mapper& dmap, Header& H) const = 0;
          virtual bool        check (Header& H, int num_axes = 0) const = 0;
          virtual void        create (Mapper& dmap, const Header& H) const = 0;

      };

    }
  }
}


#endif

