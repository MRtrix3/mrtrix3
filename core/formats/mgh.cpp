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

#include "header.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mgh.h"
#include "image_io/default.h"
#include "formats/list.h"
#include "raw.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> MGH::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".mgh"))
        return std::unique_ptr<ImageIO::Base>();

      std::ifstream in (H.name(), std::ios_base::binary);
      File::MGH::read_header (H, in);

      // Remaining header items appear AFTER the data
      // It's possible that these data may not even be there; need to make sure that we don't go over the file size
      in.seekg (MGH_DATA_OFFSET + footprint (H));
      File::MGH::read_other (H, in);

      in.close();

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }





    bool MGH::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".mgh")) return (false);
      return File::MGH::check (H, num_axes);
    }





    std::unique_ptr<ImageIO::Base> MGH::create (Header& H) const
    {
      File::OFStream out (H.name(), std::ios_base::binary);
      File::MGH::write_header (H, out);
      out.seekp (MGH_DATA_OFFSET + footprint(H));
      File::MGH::write_other (H, out);

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }

  }
}

