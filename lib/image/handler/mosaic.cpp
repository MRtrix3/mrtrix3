#include <limits>

#include "app.h"
#include "progressbar.h"
#include "image/header.h"
#include "image/handler/mosaic.h"
#include "image/utils.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {


      void Mosaic::load ()
      {
        if (files.empty())
          throw Exception ("no files specified in header for image \"" + name + "\"");

        assert (datatype.bits() > 1);

        size_t bytes_per_segment = datatype.bytes() * segsize;
        if (files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + name + "\" is larger than maximum accessible memory");

        DEBUG ("loading mosaic image \"" + name + "\"...");
        addresses.resize (1);
        addresses[0] = new uint8_t [files.size() * bytes_per_segment];
        if (!addresses[0])
          throw Exception ("failed to allocate memory for image \"" + name + "\"");

        ProgressBar progress ("reformatting DICOM mosaic images...", slices*files.size());
        uint8_t* data = addresses[0];
        for (size_t n = 0; n < files.size(); n++) {
          File::MMap file (files[n], false, false, m_xdim * m_ydim * datatype.bytes());
          size_t nx = 0, ny = 0;
          for (size_t z = 0; z < slices; z++) {
            size_t ox = nx*xdim;
            size_t oy = ny*ydim;
            for (size_t y = 0; y < ydim; y++) {
              memcpy (data, file.address() + datatype.bytes() * (ox + m_xdim* (y+oy)), xdim * datatype.bytes());
              data += xdim * datatype.bytes();
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

    }
  }
}


