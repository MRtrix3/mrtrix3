/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include <limits>
#include <unistd.h>

#include "app.h"
#include "header.h"
#include "image_io/pipe.h"

namespace MR
{
  namespace ImageIO
  {


    void Pipe::load (const Header& header, size_t)
    {
      assert (files.size() == 1);
      DEBUG ("mapping piped image \"" + files[0].name + "\"...");

      segsize /= files.size();
      int64_t bytes_per_segment = (header.datatype().bits() * segsize + 7) / 8;

      if (double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
        throw Exception ("image \"" + header.name() + "\" is larger than maximum accessible memory");

      mmap.reset (new File::MMap (files[0], writable, !is_new, bytes_per_segment));
      addresses.resize (1);
      addresses[0].reset (mmap->address());
    }


    void Pipe::unload (const Header&)
    {
      if (mmap) {
        mmap.reset();
        if (is_new)
          std::cout << files[0].name << "\n";
        addresses[0].release();
      }

      if (!is_new && files.size() == 1) {
        DEBUG ("deleting piped image file \"" + files[0].name + "\"...");
        unlink (files[0].name.c_str());
        App::signal_handler -= files[0].name;
      }

    }

  }
}


