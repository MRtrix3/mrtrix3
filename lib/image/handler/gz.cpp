/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 21/08/09.

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

#include <limits>

#include "app.h"
#include "progressbar.h"
#include "image/header.h"
#include "image/handler/gz.h"
#include "image/misc.h"
#include "file/gz.h"

#define BYTES_PER_ZCALL 524288

namespace MR
{
  namespace Image
  {
    namespace Handler
    {

      void GZ::load ()
      {
        const std::vector<File::Entry>& files (H.get_files());
        if (files.empty())
          throw Exception ("no files specified in header for image \"" + H.name() + "\"");

        segsize = Image::voxel_count (H) / files.size();
        bytes_per_segment = (H.datatype().bits() * segsize + 7) / 8;
        if (files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + H.name() + "\" is larger than maximum accessible memory");

        debug ("loading image \"" + H.name() + "\"...");
        addresses.resize (H.datatype().bits() == 1 && files.size() > 1 ? files.size() : 1);
        addresses[0] = new uint8_t [files.size() * bytes_per_segment];
        if (!addresses[0])
          throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        if (is_new)
          memset (addresses[0], 0, files.size() * bytes_per_segment);
        else {
          ProgressBar progress ("uncompressing image \"" + H.name() + "\"...",
                                files.size() * bytes_per_segment / BYTES_PER_ZCALL);
          for (size_t n = 0; n < files.size(); n++) {
            File::GZ zf (files[n].name, "rb");
            zf.seek (files[n].start);
            uint8_t* address = addresses[0] + n*bytes_per_segment;
            uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
            while (address < last) {
              zf.read (reinterpret_cast<char*> (address), BYTES_PER_ZCALL);
              address += BYTES_PER_ZCALL;
              ++progress;
            }
            last += BYTES_PER_ZCALL;
            zf.read (reinterpret_cast<char*> (address), last - address);
          }
        }

        if (addresses.size() > 1)
          for (size_t n = 1; n < addresses.size(); n++)
            addresses[n] = addresses[0] + n*bytes_per_segment;
        else
          segsize = std::numeric_limits<size_t>::max();
      }



      void GZ::unload ()
      {
        if (addresses.size()) {
          assert (addresses[0]);
          const std::vector<File::Entry>& files (H.get_files());

          if (H.readwrite()) {
            ProgressBar progress ("compressing image \"" + H.name() + "\"...",
                                  files.size() * bytes_per_segment / BYTES_PER_ZCALL);
            for (size_t n = 0; n < files.size(); n++) {
              assert (files[n].start == int64_t (lead_in_size));
              File::GZ zf (files[n].name, "wb");
              if (lead_in)
                zf.write (reinterpret_cast<const char*> (lead_in), lead_in_size);
              uint8_t* address = addresses[0] + n*bytes_per_segment;
              uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
              while (address < last) {
                zf.write (reinterpret_cast<const char*> (address), BYTES_PER_ZCALL);
                address += BYTES_PER_ZCALL;
                ++progress;
              }
              last += BYTES_PER_ZCALL;
              zf.write (reinterpret_cast<const char*> (address), last - address);
            }
          }

        }
      }




    }
  }
}


