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

#include "file/utils.h"
#include "file/path.h"
#include "header.h"
#include "image_io/pipe.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> Pipe::read (Header& H) const
    {
      if (H.name() == "-") {
        std::string name;
        getline (std::cin, name);
        H.name() = name;
      }
      else {
        if (!File::is_tempfile (H.name())) 
          return std::unique_ptr<ImageIO::Base>();
      }

      if (H.name().empty())
        throw Exception ("no filename supplied to standard input (broken pipe?)");

      if (!Path::has_suffix (H.name(), ".mif"))
        throw Exception ("MRtrix only supports the .mif format for command-line piping");

      std::unique_ptr<ImageIO::Base> original_handler (mrtrix_handler.read (H));
      std::unique_ptr<ImageIO::Pipe> io_handler (new ImageIO::Pipe (std::move (*original_handler)));
      return std::move (io_handler);
    }





    bool Pipe::check (Header& H, size_t num_axes) const
    {
      if (H.name() != "-")
        return false;

      H.name() = File::create_tempfile (0, "mif");

      return mrtrix_handler.check (H, num_axes);
    }




    std::unique_ptr<ImageIO::Base> Pipe::create (Header& H) const
    {
      std::unique_ptr<ImageIO::Base> original_handler (mrtrix_handler.create (H));
      std::unique_ptr<ImageIO::Pipe> io_handler (new ImageIO::Pipe (std::move (*original_handler)));
      return std::move (io_handler);
    }


  }
}

