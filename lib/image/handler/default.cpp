/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 05/08/09.

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
        if (H.readwrite && !files.size()) {
          assert (mem);
          off64_t bps = (H.datatype().bits() * segsize + 7) / 8;

          for (size_t n = 0; n < H.files.size(); n++) {
            File::MMap file (H.files[n], true, bps);
            memcpy (segment ? segment[n] : mem + n*bps, file.address(), bps);
          }
        }
        for (size_t n = 0; n < mem.size(); n++)
          delete [] mem[n];
      }


      void Default::map (std::vector<uint8_t>& addresses) 
      {
        /* TODO
        if (H.files.empty()) throw Exception ("no files specified in header for image \"" + H.name() + "\"");
        assert (H.handler);
        H.handler->get_addresses (segment);

        segsize = H.datatype().is_complex() ? 2 : 1;
        for (size_t i = 0; i < H.ndim(); i++) segsize *= H.dim(i); 
        segsize /= segment.size();
        assert (segsize * segment.size() == voxel_count (H));

        off64_t bps = (H.datatype().bits() * segsize + 7) / 8;
        if (H.files.size() * bps > std::numeric_limits<size_t>::max())
          throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        debug ("mapping image \"" + H.name() + "\"...");

        if (H.files.size() > MAX_FILES_PER_IMAGE) {
          mem = new uint8_t [H.files.size() * bps];
          if (!mem) throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

          if (H.files_initialised) {
            for (size_t n = 0; n < H.files.size(); n++) {
              File::MMap file (H.files[n], false, bps);
              memcpy (mem + n*bps, file.address(), bps);
            }
          }
          else memset (mem, 0, H.files.size() * bps);

          if (H.datatype().bits() == 1 && H.files.size() > 1) {
            segment = new uint8_t* [H.files.size()];
            for (size_t n = 0; n < H.files.size(); n++) 
              segment[n] = mem + n*bps;
          }
        }
        else {
          files.resize (H.files.size());
          for (size_t n = 0; n < H.files.size(); n++) {
            files[n] = new File::MMap (H.files[n], H.readwrite, bps); 
          }
          if (files.size() > 1) {
            segment = new uint8_t* [H.files.size()];
            for (size_t n = 0; n < H.files.size(); n++) 
              segment[n] = files[n]->address();
          }
          else mem = files.front()->address();
        }
        */

      }

    }
  }
}


