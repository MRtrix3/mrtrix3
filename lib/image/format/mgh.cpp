/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 29/09/12.

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

#include "file/path.h"
#include "file/utils.h"
#include "file/mgh_utils.h"
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

      Handler::Base* MGH::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mgh"))
          return NULL;
        File::MMap fmap (H.name());
        size_t data_offset = File::MGH::read_header (H, * ( (const mgh_header*) fmap.address()), fmap.size());

        Ptr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), data_offset));

        return handler.release();
      }





      bool MGH::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".mgh")) return (false);
        if (num_axes < 3) throw Exception ("cannot create MGH image with less than 3 dimensions");
        if (num_axes > 4) throw Exception ("cannot create MGH image with more than 4 dimensions");

        H.set_ndim (num_axes);

        return (true);
      }





      Handler::Base* MGH::create (Header& H) const
      {
        if (H.ndim() > 4)
          throw Exception ("MGH format cannot support more than 4 dimensions for image \"" + H.name() + "\"");

        mgh_header MGHH;
        mgh_other MGHO;
        File::MGH::write_header (MGHH, H);
        File::MGH::write_other (MGHO, H);

        File::create (H.name());

        std::ofstream out (H.name().c_str());
        if (!out) throw Exception ("error opening file \"" + H.name() + "\" for writing: " + strerror (errno));
        out.write ( (char*) &MGHH, MGH_HEADER_SIZE);
        out.close();

        File::resize (H.name(), MGH_DATA_OFFSET + Image::footprint(H));

        File::MGH::write_other_to_file (H.name(), MGHO);

        Ptr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), MGH_DATA_OFFSET));

        return handler.release();
      }

    }
  }
}

