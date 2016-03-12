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

#ifndef __file_mmap_h__
#define __file_mmap_h__

#include <iostream>
#include <cassert>
#include <stdint.h>

#include "types.h"
#include "file/entry.h"

namespace MR
{
  namespace File
  {

    class MMap : protected Entry
    {
      public:
        //! create a new memory-mapping to file in \a entry
        /*! map file in \a entry at the offset in \a entry. By default, the
         * file will be mapped read-only. If \a readwrite is set to true, a
         * write-back RAM buffer will be allocated to store the contents of the
         * file, and written back when the constructor is invoked. 
         *
         * By default, the contents of a file mapped read-write will be
         * preloaded into the RAM buffer. If the file has just been created, \a
         * preload can be set to false to prevent preloading its contents into
         * the buffer. 
         *
         * By default, the whole file is mapped. If \a mapped_size is
         * non-zero, then only the region of size \a mapped_size starting from
         * the byte offset specified in \a entry will be mapped. 
         */
        MMap (const Entry& entry, bool readwrite = false, bool preload = true, int64_t mapped_size = -1);
        ~MMap ();

        std::string name () const {
          return Entry::name;
        }
        int64_t size () const {
          return msize;
        }
        uint8_t* address() {
          return first;
        }
        const uint8_t* address() const {
          return first;
        }

        bool is_read_write () const {
          return readwrite;
        }
        bool changed () const;

        friend std::ostream& operator<< (std::ostream& stream, const MMap& m) {
          stream << "File::MMap { " << m.name() << " [" << m.fd << "], size: "
                 << m.size() << ", mapped " << (m.readwrite ? "RW" : "RO")
                 << " at " << (void*) m.address() << ", offset " << m.start << " }";
          return stream;
        }

      protected:
        int       fd;
        uint8_t*  addr;        /**< The address in memory where the file has been mapped. */
        uint8_t*  first;       /**< The address in memory to the start of the region of interest. */
        int64_t   msize;       /**< The size of the file. */
        time_t    mtime;       /**< The modification time of the file at the last check. */
        bool      readwrite;

        void map ();

      private:
        MMap (const MMap& mmap) : Entry (mmap), fd (0), addr (NULL), first (NULL), msize (0), mtime (0), readwrite (false) {
          assert (0);
        }
    };


  }
}

#endif

