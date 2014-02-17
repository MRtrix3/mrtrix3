#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "file/mgh_utils.h"
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


      RefPtr<Handler::Base> MGZ::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mgh.gz") && !Path::has_suffix (H.name(), ".mgz"))
          return RefPtr<Handler::Base>();

        mgh_header MGHH;

        File::GZ zf (H.name(), "rb");
        zf.read (reinterpret_cast<char*> (&MGHH), MGH_HEADER_SIZE);

        bool is_BE = File::MGH::read_header (H, MGHH);

        try {

          mgh_other  MGHO;
          memset (&MGHO, 0x00, 5 * sizeof(float));
          MGHO.tags.clear();

          zf.seek (MGH_DATA_OFFSET + Image::footprint (H));
          zf.read (reinterpret_cast<char*> (&MGHO), 5 * sizeof(float));

          try {

            do {
              std::string tag = zf.getline();
              if (!tag.empty())
                MGHO.tags.push_back (tag);
            } while (!zf.eof());

          } catch (...) { }

          File::MGH::read_other (H, MGHO, is_BE);

        } catch (...) { }

        zf.close();

        RefPtr<Handler::Base> handler (new Handler::GZ (H, MGH_DATA_OFFSET));
        handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

        return handler;
      }





      bool MGZ::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".nii.gz") && !Path::has_suffix (H.name(), ".mgz")) return false;
        if (num_axes < 3) throw Exception ("cannot create MGZ image with less than 3 dimensions");
        if (num_axes > 4) throw Exception ("cannot create MGZ image with more than 4 dimensions");

        H.set_ndim (num_axes);

        return true;
      }





      RefPtr<Image::Handler::Base> MGZ::create (Header& H) const
      {
        if (H.ndim() > 4)
          throw Exception ("MGZ format cannot support more than 4 dimensions for image \"" + H.name() + "\"");

        RefPtr<Handler::GZ> handler (new Handler::GZ (H, MGH_DATA_OFFSET));

        File::MGH::write_header (*reinterpret_cast<mgh_header*> (handler->header()), H);

        // Figure out how to write the post-data header information to the zipped file
        // This is not possible without implementation of a dedicated handler
        // Not worth the effort, unless a use case arises where this information absolutely
        //   must be written

        File::create (H.name());
        handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

        return handler;
      }

    }
  }
}

