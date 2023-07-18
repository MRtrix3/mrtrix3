/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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

