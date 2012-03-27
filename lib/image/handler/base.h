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
#include "datatype.h"
#include "file/entry.h"

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
          Base (const Image::Header& header);
          virtual ~Base ();

          void open ();
          void close ();

          void set_readwrite (bool readwrite) {
            writable = readwrite;
          }
          void set_image_is_new (bool image_is_new) {
            is_new = image_is_new;
          }

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

          std::vector<File::Entry> files;

          void set_name (const std::string& image_name) {
            name = image_name;
          }

          void merge (const Base& B) {
            assert (addresses.empty());
            assert (datatype == B.datatype);
            for (size_t n = 0; n < B.files.size(); ++n) 
              files.push_back (B.files[n]);
            segsize += B.segsize;
          }

        protected:
          std::string name;
          const DataType datatype;
          size_t segsize;
          VecPtr<uint8_t,true> addresses;
          bool is_new, writable;

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

