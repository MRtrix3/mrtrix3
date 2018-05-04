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

      ProgressBar progress ("reformatting DICOM mosaic images", slices*files.size());
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


