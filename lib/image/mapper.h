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

#ifndef __image_mapper_h__
#define __image_mapper_h__

#include "data_type.h"
#include "file/mmap.h"
#include "image/header.h"
#include "image/format/base.h"
#include "math/complex.h"

namespace MR {
  namespace Dialog { class File; }
  namespace Image {

    class Header;

    class Mapper {
      public:
        void  reset ();
        void  add (const std::string& filename, size_t offset = 0, size_t desired_size_if_inexistant = 0);
        void  add (const File::MMap& fmap, size_t offset = 0);
        void  add (uint8_t* memory_buffer) { assert (mem == NULL); assert (list.size() == 0); mem = memory_buffer; }


        float32 real (size_t offset) const          { return (get_val<0> (offset)); }
        void    real (float32 val, size_t offset)   { set_val<0> (val, offset); }
        float32 imag (size_t offset) const          { return (get_val<1> (offset)); }
        void    imag (float32 val, size_t offset)   { set_val<1> (val, offset); }

        void        set_temporary (bool temp) { temporary = temp; }
        std::string output_name;

      protected:
        Mapper () : 
          mem (NULL), segment (NULL), segsize (0), optimised (false),
          temporary (false), files_new (true), get_func (NULL), put_func (NULL) { }

        ~Mapper () { 
          if (mem && list.size()) throw Exception ("Mapper destroyed before committing data to file!"); 
          if (output_name.size()) std::cout << output_name << "\n";
        }

        class Entry {
          public:
            File::MMap fmap;
            size_t    offset;
            uint8_t*    start () const { return ((uint8_t*) fmap.address() + offset); }
            friend std::ostream& operator<< (std::ostream& stream, const Entry& m)
            {
              stream << "Mapper::Entry: offset = " << m.offset << ", " << m.fmap;
              return (stream);
            }
        };

        std::vector<Entry>    list;
        uint8_t*              mem;
        uint8_t**             segment;
        size_t                segsize;

        bool                  optimised, temporary, files_new;

        void                  set_data_type (DataType dt);

        //! make all files in the list read-only or read/write.
        /** Makes all files currently in the list read-only or read/write.  */
        void                  set_read_only (bool read_only) {
          for (uint s = 0; s < list.size(); s++) {
            list[s].fmap.set_read_only (read_only); 
            if (segment) segment[s] = list[s].start();
          }
        }

        const Entry&          operator[] (uint index) const { return (list[index]); }
        Entry&                operator[] (uint index)       { return (list[index]); }

        float32               (*get_func) (const void* data, size_t i);
        void                  (*put_func) (float32 val, void* data, size_t i);

        static float32        getBit       (const void* data, size_t i);
        static float32        getInt8      (const void* data, size_t i);
        static float32        getUInt8     (const void* data, size_t i);
        static float32        getInt16LE   (const void* data, size_t i);
        static float32        getUInt16LE  (const void* data, size_t i);
        static float32        getInt16BE   (const void* data, size_t i);
        static float32        getUInt16BE  (const void* data, size_t i);
        static float32        getInt32LE   (const void* data, size_t i);
        static float32        getUInt32LE  (const void* data, size_t i);
        static float32        getInt32BE   (const void* data, size_t i);
        static float32        getUInt32BE  (const void* data, size_t i);
        static float32        getFloat32LE (const void* data, size_t i);
        static float32        getFloat32BE (const void* data, size_t i);
        static float32        getFloat64LE (const void* data, size_t i);
        static float32        getFloat64BE (const void* data, size_t i);

        static void           putBit       (float32 val, void* data, size_t i);
        static void           putInt8      (float32 val, void* data, size_t i);
        static void           putUInt8     (float32 val, void* data, size_t i);
        static void           putInt16LE   (float32 val, void* data, size_t i);
        static void           putUInt16LE  (float32 val, void* data, size_t i);
        static void           putInt16BE   (float32 val, void* data, size_t i);
        static void           putUInt16BE  (float32 val, void* data, size_t i);
        static void           putInt32LE   (float32 val, void* data, size_t i);
        static void           putUInt32LE  (float32 val, void* data, size_t i);
        static void           putInt32BE   (float32 val, void* data, size_t i);
        static void           putUInt32BE  (float32 val, void* data, size_t i);
        static void           putFloat32LE (float32 val, void* data, size_t i);
        static void           putFloat32BE (float32 val, void* data, size_t i);
        static void           putFloat64LE (float32 val, void* data, size_t i);
        static void           putFloat64BE (float32 val, void* data, size_t i);


        void                  map (const Header& H);
        void                  unmap (const Header& H);
        bool                  is_mapped () const { return (segment); }

      private:

        template <off64_t inc> inline float32 get_val (off64_t offset) const {
          if (optimised) return (((float32*) segment[0])[offset + inc]);
          ssize_t nseg (offset/segsize);
          return (get_func (segment[nseg], offset - nseg*segsize + inc)); 
        }

        template <off64_t inc> inline void set_val (float32 val, off64_t offset) {
          if (optimised) ((float32*) segment[0])[offset+inc] = val;
          ssize_t nseg (offset/segsize);
          put_func (val, segment[nseg], offset - nseg*segsize + inc); 
        }

        friend class Object;
        friend class Dialog::File;
        friend class Voxel;
        friend std::ostream& operator<< (std::ostream& stream, const Mapper& dmap);
    };









    inline void Mapper::reset ()
    {
      list.clear();
      segsize = 0; 
      get_func = NULL;
      put_func = NULL;
      optimised = temporary = false;
      files_new = true;
      output_name.clear();
      if (mem) delete [] mem;
      if (segment) delete [] segment;
      mem = NULL;
      segment = NULL;
    }

    inline void Mapper::add (const std::string& filename, size_t offset, size_t desired_size_if_inexistant)
    {
      Entry entry;
      entry.fmap.init (filename, desired_size_if_inexistant, "tmp"); 
      if (entry.fmap.is_read_only()) files_new = false;
      entry.offset = offset;
      list.push_back (entry);
    }

    inline void Mapper::add (const File::MMap& fmap, size_t offset)
    {
      assert (!fmap.is_mapped());
      Entry entry;
      entry.fmap = fmap;
      if (entry.fmap.is_read_only()) files_new = false;
      entry.offset = offset;
      list.push_back (entry);
    }


  }
}

#endif


