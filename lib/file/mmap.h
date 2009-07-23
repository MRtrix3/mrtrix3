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

namespace MR {
  namespace File {

    class MMap {
      public:
        MMap (const std::string& fname, bool readonly = true, off64_t from = 0, off64_t to = off64_t(-1));
        ~MMap ();

        std::string  name () const  { return (filename); }
        off64_t      filesize () const  { return (fsize); }
        off64_t      size () const  { return (end-start); }
        template <class T> T* address () const { return (static_cast<T*> (addr)); }

        bool is_ready () const                  { return (fsize); }
        bool is_mapped () const                 { return (addr); }
        bool is_read_only () const              { return (read_only); }

        void map ();
        void map (off64_t from, off64_t to) { set_range (from, to);  map(); }
        void unmap ();

        void set_read_only (bool is_read_only) {
          if (read_only == is_read_only) return; 
          bool was_mapped = ( addr != NULL );
          unmap(); 
          read_only = is_read_only; 
          if (was_mapped) map();
        }

        bool changed () const;

        friend std::ostream& operator<< (std::ostream& stream, const MMap& m) {
          stream << "MMap { " << m.filename << " [" << m.fd << "], size " << m.fsize << ", mapped at " << m.addr << " }";
          return (stream);
        }

        static const off64_t  pagesize;

      protected:
        int               fd;
        std::string       filename;     /**< The name of the file. */
        uint8_t*          mapping_addr; /**< The address in memory where the file has been mapped. */
        uint8_t*          addr;         /**< The address in memory where the data starts. */
        off64_t           fsize;        /**< The size of the file. */
        off64_t           start;        /**< The byte offset to the start of the desired range. */
        off64_t           end;          /**< The byte offset to the end of the desired range. */
        bool              read_only;    /**< A flag to indicate whether the file is mapped as read-only. */
        time_t            mtime;        /**< The modification time of the file at the last check. */


        void set_range (off64_t from, off64_t to) {
          assert (from < to); 
          start = from; 
          end = to;
        }

      private:
        MMap (const MMap& mmap) { assert (0); }
    };


  }
}

#endif

