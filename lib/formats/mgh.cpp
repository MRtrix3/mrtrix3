/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "header.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mgh_utils.h"
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
      File::MMap fmap (H.name());
      bool is_BE = File::MGH::read_header (H, * ( (const mgh_header*) fmap.address()));

      // Remaining header items appear AFTER the data
      // It's possible that these data may not even be there; need to make sure that we don't go over the file size
      int64_t other_offset = MGH_DATA_OFFSET + footprint (H);
      const int64_t other_floats_size = 5 * sizeof(float);
      const int64_t other_tags_offset = other_offset + other_floats_size;
      if (other_offset + other_floats_size <= fmap.size()) {

        mgh_other MGHO;
        memcpy (&MGHO, fmap.address() + other_offset, other_floats_size);
        File::MGH::read_other (H, MGHO, is_BE);

        MGHO.tags.clear();
        uint8_t* p_current = fmap.address() + other_tags_offset;

        while (p_current < fmap.address() + fmap.size()) {

          int32_t tag = Raw::fetch_BE<int32_t> (p_current);
          int64_t size = Raw::fetch_BE<int64_t> (p_current+4);
          if (size & p_current[12]) 
            add_line (H.keyval()["comments"], "[MGH TAG "+str(tag) + "]: "   + (const char*) (p_current+12));

          p_current += 12 + size;
        }


      } // End reading other data beyond the end of the image data

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }





    bool MGH::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".mgh")) return (false);
      if (num_axes < 3) throw Exception ("cannot create MGH image with less than 3 dimensions");
      if (num_axes > 4) throw Exception ("cannot create MGH image with more than 4 dimensions");

      H.ndim() = num_axes;

      return (true);
    }





    std::unique_ptr<ImageIO::Base> MGH::create (Header& H) const
    {
      if (H.ndim() > 4)
        throw Exception ("MGH format cannot support more than 4 dimensions for image \"" + H.name() + "\"");

      if (H.datatype().bytes() > 1) {
        H.datatype().set_flag (DataType::BigEndian);
        H.datatype().unset_flag (DataType::LittleEndian);
      }

      mgh_header MGHH;
      mgh_other  MGHO;
      memset (&MGHH, 0x00, MGH_HEADER_SIZE);
      memset (&MGHO, 0x00, 5 * sizeof(float));
      MGHO.tags.clear();
      File::MGH::write_header (MGHH, H);
      File::MGH::write_other  (MGHO, H);

      File::OFStream out (H.name());
      out.write ( (char*) &MGHH, MGH_HEADER_SIZE);
      out.close();

      File::resize (H.name(), MGH_DATA_OFFSET + footprint(H));

      out.open (H.name(), std::ios_base::out | std::ios_base::app);
      out.write ((char*) &MGHO, 5 * sizeof (float));
      for (const auto& tag : MGHO.tags) {
        out.write (reinterpret_cast<const char*> (&std::get<0>(tag)), sizeof(int32_t));
        out.write (reinterpret_cast<const char*> (&std::get<1>(tag)), sizeof(int64_t));
        out.write (std::get<2>(tag).c_str(), std::get<2>(tag).size());
      }
      out.close();

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }

  }
}

