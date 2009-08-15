/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __file_mmap_h__
#define __file_mmap_h__

#include <iostream>
#include <cassert>
#include <stdint.h>

#include "file/entry.h"

namespace MR {
  namespace File {

    class MMap : protected Entry {
      public:
        MMap (const Entry& entry, bool read_write = false, off64_t mapped_size = -1) : 
          Entry (entry), msize (mapped_size), readwrite (read_write) { map(); }
        MMap (const std::string& fname, bool read_write = false, off64_t from = 0, off64_t mapped_size = -1) :
          Entry (fname, from), msize (mapped_size), readwrite (read_write) { map(); }
        ~MMap () { unmap(); }

        std::string     name () const        { return (Entry::name); }
        off64_t         size () const        { return (msize); }
        uint8_t*        address()            { return (addr + start); }
        const uint8_t*  address() const      { return (addr + start); }

        bool is_read_write () const           { return (readwrite); }
        bool changed () const;

        friend std::ostream& operator<< (std::ostream& stream, const MMap& m) {
          stream << "File::MMap { " << m.name() << " [" << m.fd << "], size: "
            << m.size() << ", mapped " << ( m.readwrite ? "RW" : "RO" ) 
            << " at " << (void*) m.address() << ", offset " << m.start << " }";
          return (stream);
        }

        static const off64_t  pagesize;

      protected:
        int       fd;
        uint8_t*  addr;        /**< The address in memory where the file has been mapped. */
        off64_t   msize;       /**< The size of the file. */
        time_t    mtime;       /**< The modification time of the file at the last check. */
        bool      readwrite;

        void map ();
        void unmap ();

      private:
        MMap (const MMap& mmap) : Entry (mmap) { assert (0); }
    };


  }
}

#endif

