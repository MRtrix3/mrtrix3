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
#include <unistd.h>

#include "app.h"
#include "image/header.h"
#include "image/handler/pipe.h"
#include "image/utils.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {


      void Pipe::load ()
      {
        assert (files.size() == 1);
        DEBUG ("mapping piped image \"" + files[0].name + "\"...");

        segsize /= files.size();
        int64_t bytes_per_segment = (datatype.bits() * segsize + 7) / 8;

        if (double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
          throw Exception ("image \"" + name + "\" is larger than maximum accessible memory");

        mmap = new File::MMap (files[0], writable, !is_new, bytes_per_segment);
        addresses.resize (1);
        addresses[0] = mmap->address();
      }


      void Pipe::unload()
      {
        if (mmap) {
          mmap = NULL;
          if (is_new)
            std::cout << files[0].name << "\n";
          else {
            DEBUG ("deleting piped image file \"" + files[0].name + "\"...");
            unlink (files[0].name.c_str());
          }
          addresses[0] = NULL;
        }
      }

    }
  }
}


