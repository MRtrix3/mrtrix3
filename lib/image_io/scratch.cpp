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

#include <memory>
#include <vector>

#include "image_io/scratch.h"
#include "header.h"

namespace MR
{
  namespace ImageIO
  {

    bool Scratch::is_file_backed () const { return false; }

    void Scratch::load (const Header& header, size_t buffer_size)
    {
      assert (buffer_size);
      DEBUG ("allocating scratch buffer for image \"" + header.name() + "\"...");
      try {
        addresses.push_back (std::unique_ptr<uint8_t[]> (new uint8_t [buffer_size]));
        memset (addresses[0].get(), 0, buffer_size);
      } catch (...) {
        throw Exception ("Error allocating memory for scratch buffer");
      }
    }


    void Scratch::unload (const Header& header)
    {
      if (addresses.size()) {
        DEBUG ("deleting scratch buffer for image \"" + header.name() + "\"...");
        addresses[0].reset();
      }
    }

  }
}



