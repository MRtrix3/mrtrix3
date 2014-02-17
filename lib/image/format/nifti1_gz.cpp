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

