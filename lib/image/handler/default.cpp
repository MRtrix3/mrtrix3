/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 19/08/09.

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
#include "image/header.h"
#include "image/handler/default.h"
#include "image/misc.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {

      void Default::load ()
      {
        if (files.empty())
          throw Exception ("no files specified in header for image \"" + name + "\"");

        segsize /= files.size();

        if (datatype.bits() == 1) {
          bytes_per_segment = segsize/8;
          if (bytes_per_segment*8 < int64_t (segsize))
            ++bytes_per_segment;
        }
        else bytes_per_segment = datatype.bytes() * segsize;

        if (files.size() * double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
          throw Exception ("image \"" + name + "\" is larger than maximum accessible memory");

        if (files.size() > MAX_FILES_PER_IMAGE) 
          copy_to_mem ();
        else 
          map_files ();
      }




      void Default::unload ()
      {
        if (mmaps.empty() && addresses.size()) {
          assert (addresses[0]);

          if (writable) {
            for (size_t n = 0; n < files.size(); n++) {
              File::MMap file (files[n], true, bytes_per_segment);
              memcpy (file.address(), addresses[0] + n*bytes_per_segment, bytes_per_segment);
            }
          }
        }
        else {
          for (size_t n = 0; n < addresses.size(); ++n)
            addresses[n] = NULL;
          mmaps.clear();
        }
      }



      void Default::map_files ()
      {
        debug ("mapping image \"" + name + "\"...");
        mmaps.resize (files.size());
        addresses.resize (mmaps.size());
        for (size_t n = 0; n < files.size(); n++) {
          mmaps[n] = new File::MMap (files[n], writable, bytes_per_segment);
          addresses[n] = mmaps[n]->address();
        }
      }





      void Default::copy_to_mem ()
      {
        debug ("loading image \"" + name + "\"...");
        addresses.resize (files.size() > 1 && datatype.bits() *segsize != 8*size_t (bytes_per_segment) ? files.size() : 1);
        addresses[0] = new uint8_t [files.size() * bytes_per_segment];
        if (!addresses[0]) 
          throw Exception ("failed to allocate memory for image \"" + name + "\"");

        if (is_new) memset (addresses[0], 0, files.size() * bytes_per_segment);
        else {
          for (size_t n = 0; n < files.size(); n++) {
            File::MMap file (files[n], false, bytes_per_segment);
            memcpy (addresses[0] + n*bytes_per_segment, file.address(), bytes_per_segment);
          }
        }

        if (addresses.size() > 1)
          for (size_t n = 1; n < addresses.size(); n++)
            addresses[n] = addresses[0] + n*bytes_per_segment;
        else segsize = std::numeric_limits<size_t>::max();
      }

    }
  }
}


