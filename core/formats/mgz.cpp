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


#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "file/mgh_utils.h"
#include "header.h"
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

      mgh_header MGHH;

      File::GZ zf (H.name(), "rb");
      zf.read (reinterpret_cast<char*> (&MGHH), MGH_HEADER_SIZE);

      bool is_BE = File::MGH::read_header (H, MGHH);

      try {

        mgh_other  MGHO;
        memset (&MGHO, 0x00, 5 * sizeof(float));
        MGHO.tags.clear();

        zf.seek (MGH_DATA_OFFSET + footprint (H));
        zf.read (reinterpret_cast<char*> (&MGHO), 5 * sizeof(float));

        try {

          do {
            std::string tag = zf.getline();
            if (!tag.empty())
              MGHO.tags.push_back (tag);
          } while (!zf.eof());

        } catch (...) { }

        File::MGH::read_other (H, MGHO, is_BE);

      } catch (...) { }

      zf.close();

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::GZ (H, MGH_DATA_OFFSET));
      memcpy (dynamic_cast<ImageIO::GZ*>(io_handler.get())->header(), &MGHH, sizeof(mgh_header));
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }





    bool MGZ::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".mgh.gz") && !Path::has_suffix (H.name(), ".mgz")) return false;
      if (num_axes < 3) throw Exception ("cannot create MGZ image with less than 3 dimensions");
      if (num_axes > 4) throw Exception ("cannot create MGZ image with more than 4 dimensions");

      H.ndim() = num_axes;

      return true;
    }





    std::unique_ptr<ImageIO::Base> MGZ::create (Header& H) const
    {
      if (H.ndim() > 4)
        throw Exception ("MGZ format cannot support more than 4 dimensions for image \"" + H.name() + "\"");

      std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, MGH_DATA_OFFSET));

      File::MGH::write_header (*reinterpret_cast<mgh_header*> (io_handler->header()), H);

      // Figure out how to write the post-data header information to the zipped file
      // This is not possible without implementation of a dedicated io_handler
      // Not worth the effort, unless a use case arises where this information absolutely
      //   must be written

      File::create (H.name());
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return std::move (io_handler);
    }

  }
}

