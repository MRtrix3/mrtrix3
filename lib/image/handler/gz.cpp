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
#include "image/handler/gz.h"
#include "dataset/misc.h"
#include "file/gz.h"

#define BYTES_PER_ZCALL 524288

namespace MR {
  namespace Image {
    namespace Handler {

      GZ::~GZ () 
      {
        if (addresses.size()) {
          assert (addresses[0]);

          if (H.readwrite) {
            ProgressBar::init (H.files.size() * bytes_per_segment / BYTES_PER_ZCALL, "compressing image \"" + H.name() + "\"...");
            for (size_t n = 0; n < H.files.size(); n++) {
              assert (H.files[n].start == lead_in_size);
              File::GZ zf (H.files[n].name, "wb");
              if (lead_in) zf.write (reinterpret_cast<const char*> (lead_in), lead_in_size);
              uint8_t* address = addresses[0] + n*bytes_per_segment;
              uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
              while (address < last) {
                zf.write (reinterpret_cast<const char*> (address), BYTES_PER_ZCALL);
                address += BYTES_PER_ZCALL;
                ProgressBar::inc();
              }
              last += BYTES_PER_ZCALL;
              zf.write (reinterpret_cast<const char*> (address), last - address);
              ProgressBar::done();
            }
          }

          delete [] addresses[0];
        }
        delete [] lead_in;
      }




      void GZ::execute ()
      {
        if (H.files.empty()) throw Exception ("no files specified in header for image \"" + H.name() + "\"");

        segsize = DataSet::voxel_count (H) / H.files.size();
        bytes_per_segment = (H.datatype().bits() * segsize + 7) / 8;
        if (H.files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + H.name() + "\" is larger than maximum accessible memory");

        debug ("loading image \"" + H.name() + "\"...");
        addresses.resize ( H.datatype().bits() == 1 && H.files.size() > 1 ? H.files.size() : 1 );
        addresses[0] = new uint8_t [H.files.size() * bytes_per_segment];
        if (!addresses[0]) throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        if (is_new) memset (addresses[0], 0, H.files.size() * bytes_per_segment);
        else {
          ProgressBar::init (H.files.size() * bytes_per_segment / BYTES_PER_ZCALL, "uncompressing image \"" + H.name() + "\"...");
          for (size_t n = 0; n < H.files.size(); n++) {
            File::GZ zf (H.files[n].name, "rb");
            zf.seek (H.files[n].start);
            uint8_t* address = addresses[0] + n*bytes_per_segment;
            uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
            while (address < last) {
              zf.read (reinterpret_cast<char*> (address), BYTES_PER_ZCALL);
              address += BYTES_PER_ZCALL;
              ProgressBar::inc();
            }
            last += BYTES_PER_ZCALL;
            zf.read (reinterpret_cast<char*> (address), last - address);
          }
          ProgressBar::done();
        }
        
        if (addresses.size() > 1) 
          for (size_t n = 1; n < addresses.size(); n++)
            addresses[n] = addresses[0] + n*bytes_per_segment;
        else segsize = std::numeric_limits<size_t>::max();
      }


    }
  }
}


