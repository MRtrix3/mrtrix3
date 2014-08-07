/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <unistd.h>
#include <fcntl.h>

#include "image/stride.h"
#include "types.h"
#include "file/ofstream.h"
#include "file/utils.h"
#include "file/entry.h"
#include "file/path.h"
#include "file/key_value.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/default.h"
#include "image/name_parser.h"
#include "image/format/list.h"
#include "image/format/mrtrix_utils.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {


      // extensions are:
      // mih: MRtrix Image Header
      // mif: MRtrix Image File

      RefPtr<Handler::Base> MRtrix::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mih") && !Path::has_suffix (H.name(), ".mif"))
          return RefPtr<Handler::Base>();

        File::KeyValue kv (H.name(), "mrtrix image");

        read_mrtrix_header (H, kv);

        std::string fname;
        size_t offset;
        get_mrtrix_file_path (H, "file", fname, offset);

        ParsedName::List list;
        std::vector<int> num = list.parse_scan_check (fname);

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        for (size_t n = 0; n < list.size(); ++n)
          handler->files.push_back (File::Entry (list[n].name(), offset));

        return handler;
      }





      bool MRtrix::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".mih") &&
            !Path::has_suffix (H.name(), ".mif"))
          return false;

        H.set_ndim (num_axes);
        for (size_t i = 0; i < H.ndim(); i++)
          if (H.dim (i) < 1)
            H.dim(i) = 1;

        return true;
      }




      RefPtr<Handler::Base> MRtrix::create (Header& H) const
      {
        File::OFStream out (H.name(), std::ios::out | std::ios::binary);

        out << "mrtrix image\n";

        write_mrtrix_header (H, out);

        bool single_file = Path::has_suffix (H.name(), ".mif");

        int64_t offset = 0;
        out << "file: ";
        if (single_file) {
          offset = out.tellp() + int64_t(18);
          offset += ((4 - (offset % 4)) % 4);
          out << ". " << offset << "\nEND\n";
        }
        else out << Path::basename (H.name().substr (0, H.name().size()-4) + ".dat") << "\n";

        out.close();

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        if (single_file) {
          File::resize (H.name(), offset + Image::footprint(H));
          handler->files.push_back (File::Entry (H.name(), offset));
        }
        else {
          std::string data_file (H.name().substr (0, H.name().size()-4) + ".dat");
          File::create (data_file, Image::footprint(H));
          handler->files.push_back (File::Entry (data_file));
        }

        return handler;
      }


    }
  }
}
