/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 21/08/09.

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

#ifndef __image_io_mosaic_h__
#define __image_io_mosaic_h__

#include "image_io/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace ImageIO
  {

    class Mosaic : public Base
    {
      public:
        Mosaic (Header& header, size_t mosaic_xdim, size_t mosaic_ydim, size_t slice_xdim, size_t slice_ydim, size_t nslices) :
          Base (header), m_xdim (mosaic_xdim), m_ydim (mosaic_ydim),
          xdim (slice_xdim), ydim (slice_ydim), slices (nslices) { 
            segsize = header.size(0) * header.size(1) * header.size(2);
          }
        ~Mosaic () { close(); }

      protected:
        size_t m_xdim, m_ydim, xdim, ydim, slices;

        virtual void load ();
        virtual void unload ();
    };

  }
}

#endif


