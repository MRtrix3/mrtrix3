/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 18/08/09.

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

#ifndef __image_handler_base_h__
#define __image_handler_base_h__

#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <cassert>

#include "ptr.h"

#define MAX_FILES_PER_IMAGE 256U

namespace MR
{
  namespace Image
  {

    class Header;

    //! Classes responsible for actual image loading & writing
    /*! These classes are designed to provide a consistent interface for image
     * loading & writing, so that various non-trivial types of image storage
     * can be accommodated. These include compressed files, and images stored
     * as mosaic (e.g. Siemens DICOM mosaics). */
    namespace Handler
    {

      class Base
      {
        public:
          Base (const Header& header, bool image_is_new) : H (header), is_new (image_is_new) { }
          virtual ~Base ();

          void open ();
          void close ();

          uint8_t* segment (size_t n) const {
            assert (n < addresses.size());
            return addresses[n];
          }
          size_t nsegments () const {
            return addresses.size();
          }
          size_t segment_size () const {
            check();
            return segsize;
          }

        protected:
          const Header& H;
          size_t segsize;
          VecPtr<uint8_t,true> addresses;
          bool is_new;

          void check () const {
            assert (addresses.size());
          }
          virtual void load () = 0;
          virtual void unload ();
      };

    }
  }
}

#endif

