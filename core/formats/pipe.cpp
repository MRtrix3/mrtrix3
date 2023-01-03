/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <unistd.h>

#include "signal_handler.h"
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
      if (is_dash (H.name())) {
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

      if (ImageIO::Pipe::delete_piped_images)
        SignalHandler::mark_file_for_deletion (H.name());

      if (!Path::has_suffix (H.name(), ".mif"))
        throw Exception ("MRtrix only supports the .mif format for command-line piping");

      std::unique_ptr<ImageIO::Base> original_handler (mrtrix_handler.read (H));
      std::unique_ptr<ImageIO::Pipe> io_handler (new ImageIO::Pipe (std::move (*original_handler)));
      return std::move (io_handler);
    }





    bool Pipe::check (Header& H, size_t num_axes) const
    {
      if (!is_dash (H.name()))
        return false;

      if (isatty (STDOUT_FILENO))
        throw Exception ("attempt to pipe image to standard output (this will leave temporary files behind)");

      H.name() = File::create_tempfile (0, "mif");

      SignalHandler::mark_file_for_deletion (H.name());

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

