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
#include "image/handler/default.h"
#include "image/misc.h"

#define MAX_FILES_PER_IMAGE 256U

namespace MR {
  namespace Image {
    namespace Handler {

      Default::~Default () 
      {
        if (files.empty() && addresses.size()) {
          assert (addresses[0]);

          if (H.readwrite) {
            for (size_t n = 0; n < H.files.size(); n++) {
              File::MMap file (H.files[n], true, bytes_per_segment);
              memcpy (file.address(), addresses[0] + n*bytes_per_segment, bytes_per_segment);
            }
          }

          delete [] addresses[0];
        }
      }




      void Default::execute ()
      {
        if (H.files.empty()) throw Exception ("no files specified in header for image \"" + H.name() + "\"");

        segsize = voxel_count (H) / H.files.size();

        bytes_per_segment = (H.datatype().bits() * segsize + 7) / 8;
        if (H.files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + H.name() + "\" is larger than maximum accessible memory");

        if (H.files.size() > MAX_FILES_PER_IMAGE) copy_to_mem ();
        else map_files ();
      }





      void Default::map_files () 
      {
        debug ("mapping image \"" + H.name() + "\"...");
        files.resize (H.files.size());
        addresses.resize (files.size());
        for (size_t n = 0; n < H.files.size(); n++) {
          files[n] = new File::MMap (H.files[n], H.readwrite, bytes_per_segment); 
          addresses[n] = files[n]->address();
        }
      }





      void Default::copy_to_mem () 
      {
        debug ("loading image \"" + H.name() + "\"...");
        addresses.resize (H.files.size() > 1 && H.datatype().bits()*segsize != 8*bytes_per_segment ? H.files.size() : 1 );
        addresses[0] = new uint8_t [H.files.size() * bytes_per_segment];
        if (!addresses[0]) throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        if (is_new) memset (addresses[0], 0, H.files.size() * bytes_per_segment);
        else {
          for (size_t n = 0; n < H.files.size(); n++) {
            File::MMap file (H.files[n], false, bytes_per_segment);
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


