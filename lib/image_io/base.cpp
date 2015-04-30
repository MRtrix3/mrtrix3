/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 03/09/09.

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

    void Base::open (const Header& header, size_t bytes_per_element)
    {
      if (addresses.size())
        return;

      load (header, bytes_per_element);
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

