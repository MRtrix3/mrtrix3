/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifdef MRTRIX_AS_R_LIBRARY

#include "file/path.h"
#include "header.h"
#include "image_io/ram.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> RAM::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".R"))
        return std::unique_ptr<ImageIO::Base>();

      Header* R_header = (Header*) to<size_t> (H.name().substr (0, H.name().size()-2));
      H = *R_header;
      return R_header->__get_handler(); 
    }





    bool RAM::check (Header& H, size_t num_axes) const
    {
      return Path::has_suffix (H.name(), ".R");
    }




    std::unique_ptr<ImageIO::Base> RAM::create (Header& H) const
    {
      Header* R_header = (Header*) to<size_t> (H.name().substr (0, H.name().size()-2));
      *R_header = H;
      std::unique_ptr<ImageIO::RAM> io_handler (new ImageIO::RAM (H));
      R_header->__set_handler (io_handler);
      return io_handler;
    }


  }
}

#endif

