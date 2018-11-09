/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <unistd.h>
#include <fcntl.h>

#include "header.h"
#include "image_io/default.h"
#include "formats/list.h"
#include "formats/mrtrix_utils.h"
#include "file/ofstream.h"
#include "file/utils.h"
#include "file/entry.h"
#include "file/path.h"
#include "file/key_value.h"
#include "file/name_parser.h"

namespace MR
{
  namespace Formats
  {

    // extensions are:
    // mih: MRtrix Image Header
    // mif: MRtrix Image File

    std::unique_ptr<ImageIO::Base> MRtrix::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".mih") && !Path::has_suffix (H.name(), ".mif"))
        return std::unique_ptr<ImageIO::Base>();

      File::KeyValue kv (H.name(), "mrtrix image");

      read_mrtrix_header (H, kv);

      std::string fname;
      size_t offset;
      get_mrtrix_file_path (H, "file", fname, offset);

      File::ParsedName::List list;
      auto num = list.parse_scan_check (fname);

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      for (size_t n = 0; n < list.size(); ++n)
        io_handler->files.push_back (File::Entry (list[n].name(), offset));

      return io_handler;
    }





    bool MRtrix::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".mih") &&
          !Path::has_suffix (H.name(), ".mif"))
        return false;

      H.ndim() = num_axes;
      for (size_t i = 0; i < H.ndim(); i++)
        if (H.size (i) < 1)
          H.size(i) = 1;

      return true;
    }




    std::unique_ptr<ImageIO::Base> MRtrix::create (Header& H) const
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

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      if (single_file) {
        File::resize (H.name(), offset + footprint(H));
        io_handler->files.push_back (File::Entry (H.name(), offset));
      }
      else {
        std::string data_file (H.name().substr (0, H.name().size()-4) + ".dat");
        File::create (data_file, footprint(H));
        io_handler->files.push_back (File::Entry (data_file));
      }

      return io_handler;
    }


  }
}
