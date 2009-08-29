/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/08/09.

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
#include <unistd.h>

#include "app.h"
#include "image/handler/pipe.h"
#include "image/misc.h"

namespace MR {
  namespace Image {
    namespace Handler {

      Pipe::~Pipe () 
      {
        if (file && !is_new) {
          file = NULL;
          debug ("deleting files for piped image \"" + H.name() + "\"...");
          unlink (H.files[0].name.c_str());
        }
      }




      void Pipe::execute ()
      {
        assert (H.files.size() == 1);
        debug ("mapping piped image \"" + H.name() + "\"...");

        segsize = voxel_count (H) / H.files.size();
        off64_t bytes_per_segment = (H.datatype().bits() * segsize + 7) / 8;

        if (bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + H.name() + "\" is larger than maximum accessible memory");

        file = new File::MMap (H.files[0], H.readwrite, bytes_per_segment); 
        addresses.resize(1);
        addresses[0] = file->address();
      }


    }
  }
}


