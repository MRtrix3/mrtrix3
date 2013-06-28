/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/08/09.

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

#ifdef MRTRIX_AS_R_LIBRARY

#include <limits>
#include <unistd.h>

#include "app.h"
#include "image/handler/ram.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {


      void RAM::load ()
      {
        DEBUG ("allocating RAM buffer for image \"" + name + "\"...");
        int64_t bytes_per_segment = (datatype.bits() * segsize + 7) / 8;
        addresses.push_back (new uint8_t [bytes_per_segment]);
      }


      void RAM::unload()
      {
        if (addresses.size()) {
          DEBUG ("deleting RAM buffer for image \"" + name + "\"...");
          delete [] addresses[0];
        }
      }

    }
  }
}

#endif

