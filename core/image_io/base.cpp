/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "image_io/base.h"
#include "header.h"

namespace MR
{
  namespace ImageIO
  {


    Base::Base (const Header& header) : 
      segsize (voxel_count (header)),
      is_new (false),
      writable (false) { }


    Base::~Base () { }


    bool Base::is_file_backed () const { return true; }

    void Base::open (const Header& header, size_t buffer_size)
    {
      if (addresses.size())
        return;

      load (header, buffer_size);
      DEBUG ("image \"" + header.name() + "\" loaded");
    }



    void Base::close (const Header& header)
    {
      if (addresses.empty())
        return;

      unload (header);
      DEBUG ("image \"" + header.name() + "\" unloaded");
      addresses.clear();
    }


  }
}

