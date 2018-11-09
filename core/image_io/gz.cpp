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


#include <limits>

#include "app.h"
#include "progressbar.h"
#include "header.h"
#include "image_io/gz.h"
#include "file/gz.h"

#define BYTES_PER_ZCALL 524288

namespace MR
{
  namespace ImageIO
  {

    void GZ::load (const Header& header, size_t)
    {
      if (files.empty())
        throw Exception ("no files specified in header for image \"" + header.name() + "\"");

      segsize /= files.size();
      bytes_per_segment = (header.datatype().bits() * segsize + 7) / 8;
      if (files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
        throw Exception ("image \"" + header.name() + "\" is larger than maximum accessible memory");

      DEBUG ("loading image \"" + header.name() + "\"...");
      addresses.resize (header.datatype().bits() == 1 && files.size() > 1 ? files.size() : 1);
      addresses[0].reset (new uint8_t [files.size() * bytes_per_segment]);
      if (!addresses[0])
        throw Exception ("failed to allocate memory for image \"" + header.name() + "\"");

      if (is_new)
        memset (addresses[0].get(), 0, files.size() * bytes_per_segment);
      else {
        ProgressBar progress ("uncompressing image \"" + header.name() + "\"",
            files.size() * bytes_per_segment / BYTES_PER_ZCALL);
        for (size_t n = 0; n < files.size(); n++) {
          File::GZ zf (files[n].name, "rb");
          zf.seek (files[n].start);
          uint8_t* address = addresses[0].get() + n*bytes_per_segment;
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
        //TODO this looks like it needs to be handled explicitly in unload()...
        for (size_t n = 1; n < addresses.size(); n++)
          addresses[n].reset (addresses[0].get() + n*bytes_per_segment);
      else
        segsize = std::numeric_limits<size_t>::max();
    }



    void GZ::unload (const Header& header)
    {
      if (addresses.size()) {
        assert (addresses[0]);

        if (writable) {
          ProgressBar progress ("compressing image \"" + header.name() + "\"",
              files.size() * bytes_per_segment / BYTES_PER_ZCALL);
          for (size_t n = 0; n < files.size(); n++) {
            assert (files[n].start == int64_t (lead_in_size));
            File::GZ zf (files[n].name, "wb");
            if (lead_in)
              zf.write (reinterpret_cast<const char*> (lead_in.get()), lead_in_size);
            uint8_t* address = addresses[0].get() + n*bytes_per_segment;
            uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
            while (address < last) {
              zf.write (reinterpret_cast<const char*> (address), BYTES_PER_ZCALL);
              address += BYTES_PER_ZCALL;
              ++progress;
            }
            last += BYTES_PER_ZCALL;
            zf.write (reinterpret_cast<const char*> (address), last - address);
            if (lead_out)
              zf.write (reinterpret_cast<const char*> (lead_out.get()), lead_out_size);
          }
        }

      }
    }




  }
}


