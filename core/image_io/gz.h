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


#ifndef __image_io_gz_h__
#define __image_io_gz_h__

#include "image_io/base.h"
#include "file/mmap.h"

namespace MR
{

  namespace ImageIO
  {

    class GZ : public Base
    { NOMEMALIGN
      public:
        GZ (GZ&&) = default;
        GZ (const Header& header, size_t file_header_size, size_t file_tailer_size = 0) :
          Base (header), 
          lead_in_size (file_header_size),
          lead_out_size (file_tailer_size),
          lead_in (file_header_size ? new uint8_t [file_header_size] : nullptr),
          lead_out (file_tailer_size ? new uint8_t [file_tailer_size] : nullptr) { }

        uint8_t* header () {
          return lead_in.get();
        }

        uint8_t* tailer () {
          return lead_out.get();
        }

      protected:
        int64_t  bytes_per_segment;
        size_t   lead_in_size, lead_out_size;
        std::unique_ptr<uint8_t[]> lead_in, lead_out;

        virtual void load (const Header&, size_t);
        virtual void unload (const Header&);
    };

  }
}

#endif


