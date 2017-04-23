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
      File::MGH::read_header (H, * ( (const mgh_header*) fmap.address()));

      // Remaining header items appear AFTER the data
      // It's possible that these data may not even be there; need to make sure that we don't go over the file size
      int64_t other_offset = MGH_DATA_OFFSET + footprint (H);
      const int64_t other_floats_size = 5 * sizeof(float);
      const int64_t other_tags_offset = other_offset + other_floats_size;
      if (other_offset + other_floats_size <= fmap.size()) {

        mgh_other MGHO;
        memcpy (&MGHO, fmap.address() + other_offset, other_floats_size);
        File::MGH::read_other (H, MGHO);

        uint8_t* p_current = fmap.address() + other_tags_offset;

        while (p_current < fmap.address() + fmap.size()) {
          mgh_tag tag;
          Raw::store_BE<int32_t> (0, &tag.id);
          Raw::store_BE<int64_t> (0, &tag.size);
          tag.id = *reinterpret_cast<int32_t*>(p_current);
          p_current += sizeof(int32_t);
          tag.size = *reinterpret_cast<int64_t*> (p_current);
          p_current += sizeof(int64_t);
          if (ByteOrder::BE (tag.size)) {
            std::unique_ptr<char[]> buf (new char [ByteOrder::BE (tag.size) + 1]);
            strncpy (buf.get(), reinterpret_cast<const char*> (p_current), ByteOrder::BE (tag.size));
            buf[ByteOrder::BE (tag.size)] = '\0';
            tag.content = buf.get();
            File::MGH::read_tag (H, tag);
            p_current += ByteOrder::BE (tag.size);
          }
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
        out.write (reinterpret_cast<const char*> (&tag.id), sizeof(tag.id));
        out.write (reinterpret_cast<const char*> (&tag.size), sizeof(tag.size));
        assert (size_t(ByteOrder::BE (tag.size)) == tag.content.size());
        out.write (tag.content.c_str(), tag.content.size());
      }
      out.close();

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return io_handler;
    }

  }
}

