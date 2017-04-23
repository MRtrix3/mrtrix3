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

#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "file/mgh_utils.h"
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

      mgh_header MGHH;

      File::GZ zf (H.name(), "rb");
      zf.read (reinterpret_cast<char*> (&MGHH), MGH_HEADER_SIZE);

      File::MGH::read_header (H, MGHH);

      try {

        mgh_other MGHO;
        memset (&MGHO, 0x00, 5 * sizeof(float));
        MGHO.tags.clear();

        zf.seek (MGH_DATA_OFFSET + footprint (H));
        zf.read (reinterpret_cast<char*> (&MGHO), 5 * sizeof(float));
        File::MGH::read_other (H, MGHO);

        try {

          do {
            mgh_tag tag;
            Raw::store_BE<int32_t> (0, &tag.id);
            Raw::store_BE<int64_t> (0, &tag.size);
            zf.read (reinterpret_cast<char*> (&tag.id),   sizeof(int32_t));
            zf.read (reinterpret_cast<char*> (&tag.size), sizeof(int64_t));
            if (!zf.eof() && ByteOrder::BE (tag.size) > 0) {
              std::unique_ptr<char[]> buf (new char [ByteOrder::BE (tag.size) + 1]);
              zf.read (reinterpret_cast<char*> (buf.get()), ByteOrder::BE (tag.size));
              buf[ByteOrder::BE (tag.size)] = '\0';
              tag.content = buf.get();
              File::MGH::read_tag (H, tag);
            }
          } while (!zf.eof());

        } catch (...) { }


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

      if (H.datatype().bytes() > 1) {
        H.datatype().set_flag (DataType::BigEndian);
        H.datatype().unset_flag (DataType::LittleEndian);
      }

      mgh_other MGHO;
      memset (&MGHO, 0x00, 5 * sizeof(float));
      File::MGH::write_other (MGHO, H);

      size_t lead_out_size = 5*sizeof(float);
      for (const auto& tag : MGHO.tags) {
        assert (tag.content.size() == size_t(ByteOrder::BE (tag.size)));
        lead_out_size += sizeof(tag.id) + sizeof(tag.size) + tag.content.size();
      }

      std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, MGH_DATA_OFFSET, lead_out_size));

      File::MGH::write_header (*reinterpret_cast<mgh_header*> (io_handler->header()), H);

      uint8_t* p = io_handler->tailer();
      memcpy (p, &MGHO, 5*sizeof (float));
      p += 5*sizeof(float);

      for (const auto& tag : MGHO.tags) {
        memcpy (p, &tag.id, sizeof(tag.id)); p += sizeof(tag.id);
        memcpy (p, &tag.size, sizeof(tag.size)); p += sizeof(tag.size);
        assert (size_t(ByteOrder::BE (tag.size)) == tag.content.size());
        memcpy (p, tag.content.c_str(), tag.content.size()); p += tag.content.size();
      }

      File::create (H.name());
      io_handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

      return std::move (io_handler);
    }

  }
}

