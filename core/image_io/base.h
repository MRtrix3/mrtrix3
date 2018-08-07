/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __image_io_base_h__
#define __image_io_base_h__

#include <cassert>
#include <cstdint>
#include <unistd.h>

#include "memory.h"
#include "mrtrix.h"
#include "types.h"
#include "file/entry.h"

#define MAX_FILES_PER_IMAGE 256U

namespace MR
{

  class Header;

  //! Classes responsible for actual image loading & writing
  /*! These classes are designed to provide a consistent interface for image
   * loading & writing, so that various non-trivial types of image storage
   * can be accommodated. These include compressed files, and images stored
   * as mosaic (e.g. Siemens DICOM mosaics). */
  namespace ImageIO
  {

    class Base
    { NOMEMALIGN
      public:
        Base (const Header& header);
        Base (Base&&) noexcept = default;
        Base (const Base&) = delete;
        Base& operator=(const Base&) = delete;

        virtual ~Base ();

        virtual bool is_file_backed () const;

        // buffer_size is only used for scratch data; it is ignored in all
        // other (file-backed) handlers, where the buffer size is determined
        // from the information in the header
        void open (const Header& header, size_t buffer_size = 0);
        void close (const Header& header);

        bool is_image_new () const { return is_new; }
        bool is_image_readwrite () const { return writable; }

        void set_readwrite (bool readwrite) {
          writable = readwrite;
        }
        void set_image_is_new (bool image_is_new) {
          is_new = image_is_new;
        }
        void set_readwrite_if_existing (bool readwrite) {
          if (!is_new) 
            writable = readwrite;
        }

        uint8_t* segment (size_t n) const {
          assert (n < addresses.size());
          return addresses[n].get();
        }
        size_t nsegments () const {
          return addresses.size();
        }
        size_t segment_size () const {
          check();
          return segsize;
        }

        vector<File::Entry> files;

        void merge (const Base& B) {
          assert (addresses.empty());
          for (size_t n = 0; n < B.files.size(); ++n) 
            files.push_back (B.files[n]);
          segsize += B.segsize;
        }

        friend std::ostream& operator<< (std::ostream& stream, const Base& B) {
          stream << str (B.files.size()) << " files, segsize " << str(B.segsize)
            << ", is " << (B.is_new ? "" : "NOT ") << "new, " << (B.writable ? "read/write" : "read-only");
          return stream;
        }

      protected:
        size_t segsize;
        vector<std::unique_ptr<uint8_t[]>> addresses;
        bool is_new, writable;

        void check () const {
          assert (addresses.size());
        }
        virtual void load (const Header& header, size_t buffer_size) = 0;
        virtual void unload (const Header& header) = 0;
    };

  }

}

#endif

