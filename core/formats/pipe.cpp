/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "app.h"
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

      App::signal_handler += H.name();

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

      App::signal_handler += H.name();

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

