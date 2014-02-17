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


#include "file/utils.h"
#include "file/path.h"
#include "image/header.h"
#include "image/handler/pipe.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> Pipe::read (Header& H) const
      {
        if (H.name() != "-") 
          return RefPtr<Handler::Base>();

        std::string name;
        getline (std::cin, name);
        H.name() = name;

        if (H.name().empty())
          throw Exception ("no filename supplied to standard input (broken pipe?)");

        if (!Path::has_suffix (H.name(), ".mif"))
          throw Exception ("MRtrix only supports the .mif format for command-line piping");

        RefPtr<Handler::Base> original_handler (mrtrix_handler.read (H));
        RefPtr<Handler::Pipe> handler (new Handler::Pipe (*original_handler));
        return handler;
      }





      bool Pipe::check (Header& H, size_t num_axes) const
      {
        if (H.name() != "-")
          return false;

        H.name() = File::create_tempfile (0, "mif");

        return mrtrix_handler.check (H, num_axes);
      }




      RefPtr<Handler::Base> Pipe::create (Header& H) const
      {
        RefPtr<Handler::Base> original_handler (mrtrix_handler.create (H));
        RefPtr<Handler::Pipe> handler (new Handler::Pipe (*original_handler));
        return handler;
      }


    }
  }
}

