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
