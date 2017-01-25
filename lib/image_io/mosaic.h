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


#ifndef __image_io_mosaic_h__
#define __image_io_mosaic_h__

#include "image_io/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace ImageIO
  {

    class Mosaic : public Base
    { NOMEMALIGN
      public:
        Mosaic (const Header& header, size_t mosaic_xdim, size_t mosaic_ydim, size_t slice_xdim, size_t slice_ydim, size_t nslices) :
          Base (header), m_xdim (mosaic_xdim), m_ydim (mosaic_ydim),
          xdim (slice_xdim), ydim (slice_ydim), slices (nslices) { 
            segsize = header.size(0) * header.size(1) * header.size(2);
          }

      protected:
        size_t m_xdim, m_ydim, xdim, ydim, slices;

        virtual void load (const Header&, size_t);
        virtual void unload (const Header&);
    };

  }
}

#endif


