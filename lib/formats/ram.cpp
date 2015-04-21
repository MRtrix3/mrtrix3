/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 29/08/09.

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

#ifdef MRTRIX_AS_R_LIBRARY

#include "file/path.h"
#include "image/header.h"
#include "image/handler/ram.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      std::shared_ptr<Handler::Base> RAM::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".R"))
          return std::shared_ptr<Handler::Base>();

        Header* R_header = (Header*) to<size_t> (H.name().substr (0, H.name().size()-2));
        H = *R_header;
        return R_header->__get_handler(); 
      }





      bool RAM::check (Header& H, size_t num_axes) const
      {
        return Path::has_suffix (H.name(), ".R");
      }




      std::shared_ptr<Handler::Base> RAM::create (Header& H) const
      {
        Header* R_header = (Header*) to<size_t> (H.name().substr (0, H.name().size()-2));
        *R_header = H;
        std::shared_ptr<Handler::RAM> handler (new Handler::RAM (H));
        R_header->__set_handler (handler);
        return handler;
      }


    }
  }
}

#endif

