/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 29/09/12.

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

#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mgh_utils.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/default.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> MGH::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mgh"))
          return RefPtr<Handler::Base>();
        File::MMap fmap (H.name());
        bool is_BE = File::MGH::read_header (H, * ( (const mgh_header*) fmap.address()));

        // Remaining header items appear AFTER the data
        // It's possible that these data may not even be there; need to make sure that we don't go over the file size
        int64_t other_offset = MGH_DATA_OFFSET + Image::footprint (H);
        const int64_t other_floats_size = 5 * sizeof(float);
        const int64_t other_tags_offset = other_offset + other_floats_size;
        if (other_offset + other_floats_size <= fmap.size()) {

          mgh_other MGHO;
          memcpy (&MGHO, fmap.address() + other_offset, other_floats_size);
          MGHO.tags.clear();
          if (other_tags_offset < fmap.size()) {

            // It's memory-mapped, so should be able to use memcpy to do the initial grab
            size_t total_text_length = fmap.size() - other_tags_offset;
            char* const tags = new char [total_text_length];
            memcpy (tags, &MGHO + other_tags_offset, total_text_length);

            // Extract and separate null-terminated strings
            size_t char_offset = 0;
            while (char_offset < total_text_length) {
              std::string line (tags + char_offset);
              if (line.size())
                MGHO.tags.push_back (line);
              char_offset += line.size() + 1;
            }

            delete[] tags;

          }

          File::MGH::read_other (H, MGHO, is_BE);

        } // End reading other data beyond the end of the image data

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

        return handler;
      }





      bool MGH::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".mgh")) return (false);
        if (num_axes < 3) throw Exception ("cannot create MGH image with less than 3 dimensions");
        if (num_axes > 4) throw Exception ("cannot create MGH image with more than 4 dimensions");

        H.set_ndim (num_axes);

        return (true);
      }





      RefPtr<Handler::Base> MGH::create (Header& H) const
      {
        if (H.ndim() > 4)
          throw Exception ("MGH format cannot support more than 4 dimensions for image \"" + H.name() + "\"");

        mgh_header MGHH;
        mgh_other  MGHO;
        memset (&MGHH, 0x00, MGH_HEADER_SIZE);
        memset (&MGHO, 0x00, 5 * sizeof(float));
        MGHO.tags.clear();
        File::MGH::write_header (MGHH, H);
        File::MGH::write_other  (MGHO, H);

        File::create (H.name());

        File::OFStream out (H.name());
        out.write ( (char*) &MGHH, MGH_HEADER_SIZE);
        out.close();

        File::resize (H.name(), MGH_DATA_OFFSET + Image::footprint(H));

        File::MGH::write_other_to_file (H.name(), MGHO);

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

        return handler;
      }

    }
  }
}

