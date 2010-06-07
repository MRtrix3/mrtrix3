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
#include "image/handler/mosaic.h"
#include "dataset/misc.h"

namespace MR {
  namespace Image {
    namespace Handler {

      Mosaic::~Mosaic () 
      {
        if (addresses.size()) delete [] addresses[0];
      }




      void Mosaic::execute ()
      {
        if (H.files.empty()) throw Exception ("no files specified in header for image \"" + H.name() + "\"");
        assert (H.datatype().bits() > 1);

        segsize = H.dim(0) * H.dim(1) * H.dim(2);
        assert (segsize * H.files.size() == DataSet::voxel_count (H));

        size_t bytes_per_segment = H.datatype().bytes() * segsize;
        if (H.files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + H.name() + "\" is larger than maximum accessible memory");

        debug ("loading mosaic image \"" + H.name() + "\"...");
        addresses.resize (1);
        addresses[0] = new uint8_t [H.files.size() * bytes_per_segment];
        if (!addresses[0]) throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        ProgressBar progress ("reformatting DICOM mosaic images...", slices*H.files.size()); 
        uint8_t* data = addresses[0];
        for (size_t n = 0; n < H.files.size(); n++) {
          File::MMap file (H.files[n], false, m_xdim * m_ydim * H.datatype().bytes());
          size_t nx = 0, ny = 0;
          for (size_t z = 0; z < slices; z++) {
            size_t ox = nx*xdim;
            size_t oy = ny*ydim;
            for (size_t y = 0; y < ydim; y++) {
              memcpy (data, file.address() + H.datatype().bytes()*(ox + m_xdim*(y+oy)), xdim * H.datatype().bytes());
              data += xdim * H.datatype().bytes();
            }
            nx++;
            if (nx >= m_xdim / xdim) { nx = 0; ny++; }
            ++progress;
          }
        }

        segsize = std::numeric_limits<size_t>::max();
      }

    }
  }
}


