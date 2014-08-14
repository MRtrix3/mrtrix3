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

#include <limits>
#include <unistd.h>

#include "app.h"
#include "image/header.h"
#include "image/handler/pipe.h"
#include "image/utils.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {

      Pipe::~Pipe () 
      { 
        close(); 
        if (!is_new && files.size() == 1) {
          DEBUG ("deleting piped image file \"" + files[0].name + "\"...");
          unlink (files[0].name.c_str());
        }
      }



      void Pipe::load ()
      {
        assert (files.size() == 1);
        DEBUG ("mapping piped image \"" + files[0].name + "\"...");

        segsize /= files.size();
        int64_t bytes_per_segment = (datatype.bits() * segsize + 7) / 8;

        if (double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
          throw Exception ("image \"" + name + "\" is larger than maximum accessible memory");

        mmap = new File::MMap (files[0], writable, !is_new, bytes_per_segment);
        addresses.resize (1);
        addresses[0] = mmap->address();
      }


      void Pipe::unload()
      {
        if (mmap) {
          mmap = NULL;
          if (is_new)
            std::cout << files[0].name << "\n";
          addresses[0] = NULL;
        }
      }

    }
  }
}


