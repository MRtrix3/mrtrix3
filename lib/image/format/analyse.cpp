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
#include "file/entry.h"
#include "file/nifti1_utils.h"
#include "image/header.h"
#include "image/format/list.h"
#include "image/handler/default.h"
#include "file/nifti1.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> Analyse::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".img"))
          return RefPtr<Handler::Base>();

        File::MMap fmap (H.name().substr (0, H.name().size()-4) + ".hdr");
        File::NIfTI::read (H, * ( (const nifti_1_header*) fmap.address()));

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name()));

        return handler;
      }





      bool Analyse::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".img"))
          return (false);

        if (num_axes < 3)
          throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");

        if (num_axes > 8)
          throw Exception ("cannot create NIfTI-1.1 image with more than 8 dimensions");

        H.set_ndim (num_axes);
        File::NIfTI::check (H, false);

        return (true);
      }





      RefPtr<Handler::Base> Analyse::create (Header& H) const
      {
        if (H.ndim() > 7)
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        nifti_1_header NH;
        File::NIfTI::write (NH, H, false);

        std::string hdr_name (H.name().substr (0, H.name().size()-4) + ".hdr");
        File::create (hdr_name);

        std::ofstream out (hdr_name.c_str());
        if (!out)
          throw Exception ("error opening file \"" + hdr_name + "\" for writing: " + strerror (errno));
        out.write ( (char*) &NH, 352);
        out.close();

        File::create (H.name(), Image::footprint(H));

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name()));

        return handler;
      }

    }
  }
}
