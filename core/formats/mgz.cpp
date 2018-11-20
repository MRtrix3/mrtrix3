/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "file/mgh.h"
#include "header.h"
#include "raw.h"
#include "image_io/gz.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {



    std::unique_ptr<ImageIO::Base> MGZ::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".mgh.gz") && !Path::has_suffix (H.name(), ".mgz"))
        return std::unique_ptr<ImageIO::Base>();

      std::string mgh_header (MGH_DATA_OFFSET, '\0');
      File::GZ in (H.name(), "rb");
      in.read (reinterpret_cast<char*> (&mgh_header[0]), MGH_DATA_OFFSET);
      std::istringstream s (mgh_header);
      File::MGH::read_header (H, s);

      // Remaining header items appear AFTER the data
      // It's possible that these data may not even be there; need to make sure that we don't go over the file size
      in.seek (MGH_DATA_OFFSET + footprint (H));
      File::MGH::read_other (H, in);
      in.close();

      // need to fill in GZ handler's header and tailer entries in case image
      // is opened read/write:
      std::ostringstream mgh_tailer;
      File::MGH::write_other (H, mgh_tailer);

      auto gz_handler = new ImageIO::GZ (H, MGH_DATA_OFFSET, mgh_tailer.str().size());
      memcpy (gz_handler->header(), mgh_header.c_str(), mgh_header.size());
      memcpy (gz_handler->tailer(), mgh_tailer.str().c_str(), mgh_tailer.str().size());

      std::unique_ptr<ImageIO::Base> io_handler (gz_handler);
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }





    bool MGZ::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".mgh.gz") && !Path::has_suffix (H.name(), ".mgz")) return false;
      return File::MGH::check (H, num_axes);
    }





    std::unique_ptr<ImageIO::Base> MGZ::create (Header& H) const
    {
      std::ostringstream mgh_header, mgh_tailer;
      File::MGH::write_header (H, mgh_header);
      File::MGH::write_other (H, mgh_tailer);

      File::create (H.name());
      auto gz_handler = new ImageIO::GZ (H, MGH_DATA_OFFSET, mgh_tailer.str().size());

      memset (gz_handler->header(), 0, MGH_DATA_OFFSET);
      memcpy (gz_handler->header(), mgh_header.str().c_str(), mgh_header.str().size());
      memcpy (gz_handler->tailer(), mgh_tailer.str().c_str(), mgh_tailer.str().size());

      std::unique_ptr<ImageIO::Base> io_handler (gz_handler);
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }

  }
}

