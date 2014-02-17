/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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


