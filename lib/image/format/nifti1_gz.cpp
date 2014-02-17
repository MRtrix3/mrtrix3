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
#include "file/gz.h"
#include "file/nifti1_utils.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/gz.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {


      RefPtr<Handler::Base> NIfTI_GZ::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".nii.gz")) 
          return RefPtr<Handler::Base>();

        nifti_1_header NH;

        File::GZ zf (H.name(), "rb");
        zf.read (reinterpret_cast<char*> (&NH), sizeof (nifti_1_header));
        zf.close();

        size_t data_offset = File::NIfTI::read (H, NH);

        RefPtr<Handler::Base> handler (new Handler::GZ (H, sizeof(nifti_1_header)+sizeof(nifti1_extender)));
        handler->files.push_back (File::Entry (H.name(), data_offset));

        return handler;
      }





      bool NIfTI_GZ::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".nii.gz"))
          return false;

        if (num_axes < 3)
          throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");

        if (num_axes > 8)
          throw Exception ("cannot create NIfTI-1.1 image with more than 8 dimensions");

        H.set_ndim (num_axes);
        File::NIfTI::check (H, true);

        return true;
      }





      RefPtr<Image::Handler::Base> NIfTI_GZ::create (Header& H) const
      {
        if (H.ndim() > 7)
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        RefPtr<Handler::GZ> handler (new Handler::GZ (H, sizeof(nifti_1_header)+sizeof(nifti1_extender)));

        File::NIfTI::write (*reinterpret_cast<nifti_1_header*> (handler->header()), H, true);
        memset (handler->header()+sizeof(nifti_1_header), 0, sizeof(nifti1_extender));

        File::create (H.name());
        handler->files.push_back (File::Entry (H.name(), sizeof(nifti_1_header)+sizeof(nifti1_extender)));

        return handler;
      }

    }
  }
}

