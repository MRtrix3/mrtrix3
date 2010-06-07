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

#include "file/misc.h"
#include "file/path.h"
#include "image/header.h"
#include "image/handler/pipe.h"
#include "image/format/list.h"

namespace MR {
  namespace Image {
    namespace Format {

      bool Pipe::read (Header& H) const
      { 
        if (H.name() != "-") return (false);

        getline (std::cin, H.name());

        if (H.name().empty()) 
          throw Exception ("no filename supplied to standard input (broken pipe?)");

        if (!Path::has_suffix (H.name(), ".mif")) 
          throw Exception ("MRtrix only supports the .mif format for command-line piping");

        try {
          if (mrtrix_handler.read (H)) {
            H.handler = new Handler::Pipe (H, false);
            return (true);
          }
        }
        catch (Exception& E) { throw Exception (E, "error reading image data from command-line pipe"); }
        return (false);
      }





      bool Pipe::check (Header& H, size_t num_axes) const
      {
        if (H.name() != "-") return (false);
        H.name() = File::create_tempfile (0, "mif");

        return (mrtrix_handler.check (H, num_axes));
      }




      void Pipe::create (Header& H) const
      {
        mrtrix_handler.create (H);
        H.handler = new Handler::Pipe (H, true);
      }


    }
  }
}

