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
#include "header.h"
#include "image_io/mosaic.h"

namespace MR
{
  namespace ImageIO
  {


    void Mosaic::load (const Header& header, size_t)
    {
      if (files.empty())
        throw Exception ("no files specified in header for image \"" + header.name() + "\"");

      assert (header.datatype().bits() > 1);

      size_t bytes_per_segment = header.datatype().bytes() * segsize;
      if (files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
        throw Exception ("image \"" + header.name() + "\" is larger than maximum accessible memory");

      DEBUG ("loading mosaic image \"" + header.name() + "\"...");
      addresses.resize (1);
      addresses[0].reset (new uint8_t [files.size() * bytes_per_segment]);
      if (!addresses[0])
        throw Exception ("failed to allocate memory for image \"" + header.name() + "\"");

      ProgressBar progress ("reformatting DICOM mosaic images...", slices*files.size());
      uint8_t* data = addresses[0].get();
      for (size_t n = 0; n < files.size(); n++) {
        File::MMap file (files[n], false, false, m_xdim * m_ydim * header.datatype().bytes());
        size_t nx = 0, ny = 0;
        for (size_t z = 0; z < slices; z++) {
          size_t ox = nx*xdim;
          size_t oy = ny*ydim;
          for (size_t y = 0; y < ydim; y++) {
            memcpy (data, file.address() + header.datatype().bytes() * (ox + m_xdim* (y+oy)), xdim * header.datatype().bytes());
            data += xdim * header.datatype().bytes();
          }
          nx++;
          if (nx >= m_xdim / xdim) {
            nx = 0;
            ny++;
          }
          ++progress;
        }
      }

      segsize = std::numeric_limits<size_t>::max();
    }

    void Mosaic::unload (const Header& header) { }

  }
}


