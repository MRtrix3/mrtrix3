/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include <limits>

#include "app.h"
#include "header.h"
#include "file/ofstream.h"
#include "image_io/default.h"

namespace MR
{
  namespace ImageIO
  {

    void Default::load (const Header& header, size_t)
    {
      if (files.empty())
        throw Exception ("no files specified in header for image \"" + header.name() + "\"");

      segsize /= files.size();

      if (header.datatype().bits() == 1) {
        bytes_per_segment = segsize/8;
        if (bytes_per_segment*8 < int64_t (segsize))
          ++bytes_per_segment;
      }
      else bytes_per_segment = header.datatype().bytes() * segsize;

      if (files.size() * double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
        throw Exception ("image \"" + header.name() + "\" is larger than maximum accessible memory");

      if (files.size() > MAX_FILES_PER_IMAGE) 
        copy_to_mem (header);
      else 
        map_files (header);
    }




    void Default::unload (const Header& header)
    {
      if (mmaps.empty() && addresses.size()) {
        assert (addresses[0].get());

        if (writable) {
          for (size_t n = 0; n < files.size(); n++) {
            File::OFStream out (files[n].name, std::ios::out | std::ios::binary);
            out.seekp (files[n].start, out.beg);
            out.write ((char*) (addresses[0].get() + n*bytes_per_segment), bytes_per_segment);
            if (!out.good())
              throw Exception ("error writing back contents of file \"" + files[n].name + "\": " + strerror(errno));
          }
        }
      }
      else {
        for (size_t n = 0; n < addresses.size(); ++n)
          addresses[n].release();
        mmaps.clear();
      }
    }



    void Default::map_files (const Header& header)
    {
      mmaps.resize (files.size());
      addresses.resize (mmaps.size());
      for (size_t n = 0; n < files.size(); n++) {
        mmaps[n].reset (new File::MMap (files[n], writable, !is_new, bytes_per_segment));
        addresses[n].reset (mmaps[n]->address());
      }
    }





    void Default::copy_to_mem (const Header& header)
    {
      DEBUG ("loading image \"" + header.name() + "\"...");
      addresses.resize (files.size() > 1 && header.datatype().bits() *segsize != 8*size_t (bytes_per_segment) ? files.size() : 1);
      addresses[0].reset (new uint8_t [files.size() * bytes_per_segment]);
      if (!addresses[0]) 
        throw Exception ("failed to allocate memory for image \"" + header.name() + "\"");

      if (is_new) memset (addresses[0].get(), 0, files.size() * bytes_per_segment);
      else {
        for (size_t n = 0; n < files.size(); n++) {
          File::MMap file (files[n], false, false, bytes_per_segment);
          memcpy (addresses[0].get() + n*bytes_per_segment, file.address(), bytes_per_segment);
        }
      }

      if (addresses.size() > 1)
        for (size_t n = 1; n < addresses.size(); n++)
          addresses[n].reset (addresses[0].get() + n*bytes_per_segment);
      else segsize = std::numeric_limits<size_t>::max();
    }

  }
}


