#include "file/path.h"
#include "file/utils.h"
#include "file/nifti1_utils.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/default.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> NIfTI::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".nii")) 
          return RefPtr<Handler::Base>();

        File::MMap fmap (H.name());
        size_t data_offset = File::NIfTI::read (H, * ( (const nifti_1_header*) fmap.address()));

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), data_offset));

        return handler;
      }





      bool NIfTI::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".nii")) return (false);
        if (num_axes < 3) throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");
        if (num_axes > 8) throw Exception ("cannot create NIfTI-1.1 image with more than 8 dimensions");

        H.set_ndim (num_axes);
        File::NIfTI::check (H, true);

        return true;
      }





      RefPtr<Handler::Base> NIfTI::create (Header& H) const
      {
        if (H.ndim() > 7)
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        nifti_1_header NH;
        nifti1_extender extender;
        memset (extender.extension, 0x00, sizeof (nifti1_extender));
        File::NIfTI::write (NH, H, true);

        File::create (H.name());

        std::ofstream out (H.name().c_str());
        if (!out) throw Exception ("error opening file \"" + H.name() + "\" for writing: " + strerror (errno));
        out.write ( (char*) &NH, 348);
        out.write (extender.extension, 4);
        out.close();

        File::resize (H.name(), 352 + Image::footprint(H));

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), 352));

        return handler;
      }

    }
  }
}

