/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 18/08/09.

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

#ifndef __image_handler_h__
#define __image_handler_h__

#include <map>
#include <stdint.h>

namespace MR {
  namespace Image {

    class Header;

    //! \addtogroup Image 
    // @{
    
    namespace Handler {

      class Base {
        public:
          Base (Header& header) : H (header) { }
          virtual ~Base () { }
          virtual void map (std::vector<uint8_t>& addresses) = 0; 
        protected:
          Header& H;
      };

    }
    //! @}
  }
}

#endif

