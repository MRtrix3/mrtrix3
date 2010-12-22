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
#include "dataset/misc.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {

      Default::~Default ()
      {
        if (files.empty() && addresses.size()) {
          assert (addresses[0]);

          if (H.readwrite()) {
            const std::vector<File::Entry>& Hfiles (H.get_files());
            for (size_t n = 0; n < Hfiles.size(); n++) {
              File::MMap file (Hfiles[n], true, bytes_per_segment);
              memcpy (file.address(), addresses[0] + n*bytes_per_segment, bytes_per_segment);
            }
          }

          delete [] addresses[0];
        }
      }




      void Default::execute ()
      {
        const std::vector<File::Entry>& Hfiles (H.get_files());
        if (Hfiles.empty())
          throw Exception ("no files specified in header for image \"" + H.name() + "\"");

        segsize = DataSet::voxel_count (H) / Hfiles.size();

        if (H.datatype().bits() == 1) {
          bytes_per_segment = segsize/8;
          if (bytes_per_segment*8 < int64_t (segsize))
            ++bytes_per_segment;
        }
        else bytes_per_segment = H.datatype().bytes() * segsize;

        if (Hfiles.size() * double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
          throw Exception ("image \"" + H.name() + "\" is larger than maximum accessible memory");

        if (Hfiles.size() > MAX_FILES_PER_IMAGE) copy_to_mem ();
        else map_files ();
      }





      void Default::map_files ()
      {
        debug ("mapping image \"" + H.name() + "\"...");
        const std::vector<File::Entry>& Hfiles (H.get_files());
        files.resize (Hfiles.size());
        addresses.resize (files.size());
        for (size_t n = 0; n < Hfiles.size(); n++) {
          files[n] = new File::MMap (Hfiles[n], H.readwrite(), bytes_per_segment);
          addresses[n] = files[n]->address();
        }
      }





      void Default::copy_to_mem ()
      {
        debug ("loading image \"" + H.name() + "\"...");
        const std::vector<File::Entry>& Hfiles (H.get_files());
        addresses.resize (Hfiles.size() > 1 && H.datatype().bits() *segsize != 8*size_t (bytes_per_segment) ? Hfiles.size() : 1);
        addresses[0] = new uint8_t [Hfiles.size() * bytes_per_segment];
        if (!addresses[0]) throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        if (is_new) memset (addresses[0], 0, Hfiles.size() * bytes_per_segment);
        else {
          for (size_t n = 0; n < Hfiles.size(); n++) {
            File::MMap file (Hfiles[n], false, bytes_per_segment);
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


