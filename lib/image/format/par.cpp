/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Maximilian Pietsch, 03/03/15.

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


#include "file/config.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mmap.h"
#include "image/utils.h"
#include "image/format/list.h"
#include "image/header.h"
#include "image/handler/default.h"
#include "get_set.h"



namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> PAR::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".PAR") && !Path::has_suffix (H.name(), ".par")){
          return RefPtr<Handler::Base>();
        }

        File::MMap fmap (H.name());

        // if (memcmp (fmap.address(), "PAR#", 4))
          // throw Exception ("file \"" + H.name() + "\" is not in PAR format (unrecognised magic number)");

        size_t data_offset = 0;

        if (!data_offset)
          throw Exception ("no data field found in PAR image \"" + H.name() + "\"");

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), data_offset));

        return handler;
      }


      bool PAR::check (Header& H, size_t num_axes) const
      {
        return false;
      }

      RefPtr<Handler::Base> PAR::create (Header& H) const
      {
        assert (0);
        return RefPtr<Handler::Base>();
      }

    }
  }
}



