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
#include "image/handler/gz.h"
#include "image/utils.h"
#include "file/gz.h"

#define BYTES_PER_ZCALL 524288

namespace MR
{
  namespace Image
  {
    namespace Handler
    {

      void GZ::load ()
      {
        if (files.empty())
          throw Exception ("no files specified in header for image \"" + name + "\"");

        segsize /= files.size();
        bytes_per_segment = (datatype.bits() * segsize + 7) / 8;
        if (files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
          throw Exception ("image \"" + name + "\" is larger than maximum accessible memory");

        DEBUG ("loading image \"" + name + "\"...");
        addresses.resize (datatype.bits() == 1 && files.size() > 1 ? files.size() : 1);
        addresses[0] = new uint8_t [files.size() * bytes_per_segment];
        if (!addresses[0])
          throw Exception ("failed to allocate memory for image \"" + name + "\"");

        if (is_new)
          memset (addresses[0], 0, files.size() * bytes_per_segment);
        else {
          ProgressBar progress ("uncompressing image \"" + name + "\"...",
                                files.size() * bytes_per_segment / BYTES_PER_ZCALL);
          for (size_t n = 0; n < files.size(); n++) {
            File::GZ zf (files[n].name, "rb");
            zf.seek (files[n].start);
            uint8_t* address = addresses[0] + n*bytes_per_segment;
            uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
            while (address < last) {
              zf.read (reinterpret_cast<char*> (address), BYTES_PER_ZCALL);
              address += BYTES_PER_ZCALL;
              ++progress;
            }
            last += BYTES_PER_ZCALL;
            zf.read (reinterpret_cast<char*> (address), last - address);
          }
        }

        if (addresses.size() > 1)
          for (size_t n = 1; n < addresses.size(); n++)
            addresses[n] = addresses[0] + n*bytes_per_segment;
        else
          segsize = std::numeric_limits<size_t>::max();
      }



      void GZ::unload ()
      {
        if (addresses.size()) {
          assert (addresses[0]);

          if (writable) {
            ProgressBar progress ("compressing image \"" + name + "\"...",
                                  files.size() * bytes_per_segment / BYTES_PER_ZCALL);
            for (size_t n = 0; n < files.size(); n++) {
              assert (files[n].start == int64_t (lead_in_size));
              File::GZ zf (files[n].name, "wb");
              if (lead_in)
                zf.write (reinterpret_cast<const char*> (lead_in), lead_in_size);
              uint8_t* address = addresses[0] + n*bytes_per_segment;
              uint8_t* last = address + bytes_per_segment - BYTES_PER_ZCALL;
              while (address < last) {
                zf.write (reinterpret_cast<const char*> (address), BYTES_PER_ZCALL);
                address += BYTES_PER_ZCALL;
                ++progress;
              }
              last += BYTES_PER_ZCALL;
              zf.write (reinterpret_cast<const char*> (address), last - address);
            }
          }

        }
      }




    }
  }
}


