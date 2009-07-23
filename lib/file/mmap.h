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
        MMap (const std::string& fname, off64_t desired_size_if_inexistant = 0, off64_t data_offset = 0, const char* suffix = NULL);
        ~MMap ();

        std::string  name () const  { return (filename); }
        off64_t      size () const  { return (msize); }
        template <class T> T* address () const { return (static_cast<T*> (addr)); }
        template <class T> T* data () const { return (static_cast<T*> (addr + byte_offset)); }
        off64_t      offset () const { return (byte_offset); }

        bool is_ready () const                  { return (msize); }
        bool is_mapped () const                 { return (addr); }
        bool is_read_only () const              { return (read_only); }
        bool is_marked_for_deletion () const    { return (delete_after); }

        void map ();
        void unmap ();
        void resize (off64_t new_size);

        void set_offset (off64_t new_offset) { byte_offset = new_offset; }
        void set_read_only (bool is_read_only) {
          if (read_only == is_read_only) return; 
          bool was_mapped = ( addr != NULL );
          unmap(); 
          read_only = is_read_only; 
          if (was_mapped) map();
        }
        void mark_for_deletion () { delete_after = true; }

        bool changed () const;

        friend std::ostream& operator<< (std::ostream& stream, const MMap& m) {
          stream << "MMap { " << m.filename << " [" << m.fd << "] mapped at " << m.addr << ", size " << m.msize << " }";
          return (stream);
        }

      protected:
        int               fd;
        std::string       filename;     /**< The name of the file. */
        uint8_t*          addr;         /**< The address in memory where the file has been mapped. */
        off64_t           msize;        /**< The size of the memory-mapping, should correspond to the size of the file. */
        off64_t           byte_offset;  /**< The byte offset to the start of the data, where relevant. */
        bool              read_only;    /**< A flag to indicate whether the file is mapped as read-only. */
        bool              delete_after;
        time_t            mtime;

      private:
        MMap (const MMap& mmap) { assert (0); }
    };


  }
}

#endif

