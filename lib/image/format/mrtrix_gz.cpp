#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/gz.h"
#include "image/format/list.h"
#include "image/format/mrtrix_utils.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {


      RefPtr<Handler::Base> MRtrix_GZ::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mif.gz"))
          return RefPtr<Handler::Base>();

        File::GZ zf (H.name(), "r");
        std::string first_line = zf.getline();
        if (first_line != "mrtrix image") {
          zf.close();
          throw Exception ("invalid first line for compressed image \"" + H.name() + "\" (expected \"mrtrix image\", read \"" + first_line + "\")");
        }
        read_mrtrix_header (H, zf);
        zf.close();

        std::string fname;
        size_t offset;
        get_mrtrix_file_path (H, "file", fname, offset);
        if (fname != H.name())
          throw Exception ("GZip-compressed MRtrix format images must have image data within the same file as the header");

        RefPtr<Handler::Base> handler (new Handler::GZ (H, offset));
        handler->files.push_back (File::Entry (H.name(), offset));

        return handler;
      }





      bool MRtrix_GZ::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".mif.gz"))
          return false;

        H.set_ndim (num_axes);
        for (size_t i = 0; i < H.ndim(); i++)
          if (H.dim (i) < 1)
            H.dim(i) = 1;

        return true;
      }





      RefPtr<Image::Handler::Base> MRtrix_GZ::create (Header& H) const
      {
        std::stringstream header;
        header << "mrtrix image\n";
        write_mrtrix_header (H, header);

        int64_t offset = header.tellp() + int64_t(24);
        offset += ((4 - (offset % 4)) % 4);
        header << "file: . " << offset << "\nEND\n";
        while (header.tellp() < offset)
          header << '\0';

        RefPtr<Handler::GZ> handler (new Handler::GZ (H, offset));
        memcpy (handler->header(), header.str().c_str(), offset);

        File::create (H.name());
        handler->files.push_back (File::Entry (H.name(), offset));

        return handler;
      }

    }
  }
}

