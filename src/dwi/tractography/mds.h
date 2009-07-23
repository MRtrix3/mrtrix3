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

#ifndef __dwi_tractography_mds_h__
#define __dwi_tractography_mds_h__

#include "point.h"
#include "get_set.h"
#include "file/mmap.h"
#include "dwi/tractography/mds_tags.h"
#include "dwi/tractography/mds.h"
#include "dwi/tractography/properties.h"

#define MDS_SIZE_INC 4096

/*
   MRI format:
   Magic number:              MDS#         (4 bytes)
   Byte order specifier:    uint16_t = 1     (2 bytes)
   ...
Elements:
ID:                      uint32_t       (4 bytes)
size:                    uint32_t       (4 bytes)
contents:              unspecified  ('size' bytes)
...

Datatype:      ( ID & 0x000000FFU ) 
*/

namespace MR {
  namespace DWI {
    namespace Tractography {

      class MDS {
        public:
          class Track {
            public:
              MDS*       parent;
              size_t      offset;
              uint      count;
              bool       is_BE;
              std::vector<Point> next () const
              {
                std::vector<Point> V;
                float32* data = (float32*) ((uint8_t*) parent->get_mmap().address() + offset);
                for (uint n = 0; n < count; n++) 
                  V.push_back (Point (get<float32> (data+3*n, is_BE), get<float32> (data+3*n+1, is_BE), get<float32> (data+3*n+2, is_BE) ));
                return (V);
              }
          };



          MDS () { current_offset = next = 0; }

          void         read (const std::string& filename, Properties& properties);
          std::vector<Track>  tracks;
          bool         changed () const     { return (mmap.changed()); }

          void         create (const std::string& filename, const Properties& properties);
          void         append (const std::vector<Point>& points);
          void         finalize ();

          std::string       name () const    { return (mmap.name()); }

        private:
          MR::File::MMap  mmap;
          size_t        current_offset, next;
          bool         is_BE;
          std::vector<Tag>   stack;

          uint32_t       read_uint32 (int idx) const;

        protected:

          ROI::Type type;
          uint8_t shape;
          Point sphere_pos;
          float sphere_rad;
          std::string mask_name;
          Properties* prop;


          bool         BE () const { return (is_BE); }
          void         interpret ();

          const MR::File::MMap&  get_mmap () const { return (mmap); }

          uint32_t      size () const { return (read_uint32 (1)); }
          Tag          tag () const { Tag T (read_uint32(0)); return (T); }
          uint32_t      count () const;
          size_t        offset (uint index = 0) const;
          const uint8_t* data (uint index = 0) const;
          uint8_t*       data (uint index = 0);

          std::string       get_string () const;
          int8_t        get_int8 (uint index = 0) const;
          uint8_t       get_uint8 (uint index = 0) const;
          int16_t       get_int16 (uint index = 0) const;
          uint16_t      get_uint16 (uint index = 0) const;
          int32_t       get_int32 (uint index = 0) const;
          uint32_t      get_uint32 (uint index = 0) const;
          float32      get_float32 (uint index = 0) const;
          float64      get_float64 (uint index = 0) const;

          void         append (Tag tag_id, uint32_t nbytes = 0);

          void         append_string (const Tag& T, const std::string& str);
          void         append_int8 (const Tag& T, int8_t val);
          void         append_uint8 (const Tag& T, uint8_t val);
          void         append_int32 (const Tag& T, int32_t val);
          void         append_uint32 (const Tag& T, uint32_t val);
          void         append_float32 (const Tag& T, float32 val);
          void         append_float32 (const Tag& T, const float32* val, int num);
          void         append_float64 (const Tag& T, const float64* val, int num);

          const std::vector<Tag>& containers () const;
      };












      /************************************************************************
       *            MDS inline function definitions             *
       ************************************************************************/



      inline uint32_t MDS::read_uint32 (int idx) const 
      { return (get<uint32_t> ((uint8_t*) mmap.address() + current_offset + idx*sizeof (uint32_t), is_BE)); }





      inline uint32_t MDS::count () const
      {
        if (size() == 0) return (0);
        if (tag().type() == DataType::Text) return (1);
        assert (tag().type() != DataType::Bit);
        return (size() / tag().type().bytes());
      }

      inline size_t MDS::offset (uint index) const
      {
        if (index == 0) return (current_offset + 2*sizeof (uint32_t));
        assert (tag().type() != DataType::Bit && tag().type() != DataType::Text);
        return (current_offset + 2*sizeof (uint32_t) + index*tag().type().bytes());
      }

      inline const uint8_t* MDS::data (uint index) const
      { 
        if (index == 0) return ((uint8_t*) mmap.address() + current_offset + 2*sizeof (uint32_t));
        assert (tag().type() != DataType::Bit && tag().type() != DataType::Text);
        return ((uint8_t*) mmap.address() + current_offset + 2*sizeof (uint32_t) + index*tag().type().bytes());
      }

      inline uint8_t* MDS::data (uint index)
      {
        if (index == 0) return ((uint8_t*) mmap.address() + current_offset + 2*sizeof (uint32_t));
        assert (tag().type() != DataType::Bit && tag().type() != DataType::Text);
        return ((uint8_t*) mmap.address() + current_offset + 2*sizeof (uint32_t) + index*tag().type().bytes());
      }

      inline const std::vector<Tag>& MDS::containers () const   { return (stack); }





      inline std::string MDS::get_string () const { return (std::string ((const char*) data(), size())); }
      inline int8_t MDS::get_int8 (uint index) const { return (*((int8_t*) data (index))); }
      inline uint8_t MDS::get_uint8 (uint index) const { return (*((uint8_t*) data (index))); }
      inline int16_t MDS::get_int16 (uint index) const { return (get<int16_t> (data(index), is_BE)); }
      inline uint16_t MDS::get_uint16 (uint index) const { return (get<uint16_t> (data(index), is_BE)); }
      inline int32_t MDS::get_int32 (uint index) const { return (get<int32_t> (data(index), is_BE)); }
      inline uint32_t MDS::get_uint32 (uint index) const { return (get<uint32_t> (data(index), is_BE)); }
      inline float32 MDS::get_float32 (uint index) const { return (get<float32> (data(index), is_BE)); }
      inline float64 MDS::get_float64 (uint index) const { return (get<float64> (data(index), is_BE)); }




      inline void MDS::append_string (const Tag& T, const std::string& str)
      {
        uint s = strlen (str.c_str ()); 
        append (T, s);
        memcpy (data(), str.c_str (), s); 
      }



      inline void MDS::append_int8 (const Tag& T, int8_t val)
      {
        append (T, sizeof (int8_t));
        *((int8_t*) data()) = val;
      }




      inline void MDS::append_uint8 (const Tag& T, uint8_t val)
      {
        append (T, sizeof (uint8_t));
        *((uint8_t*) data()) = val;
      }




      inline void MDS::append_int32 (const Tag& T, int32_t val)
      {
        append (T, sizeof (int32_t));
        put<int32_t> (val, data(), is_BE);
      }




      inline void MDS::append_uint32 (const Tag& T, uint32_t val)
      {
        append (T, sizeof (uint32_t));
        put<uint32_t> (val, data(), is_BE);
      }




      inline void MDS::append_float32 (const Tag& T, float32 val)
      {
        append (T, sizeof (float32));
        put<float32> (val, data(), is_BE);
      }




      inline void MDS::append_float32 (const Tag& T, const float32* val, int num)
      {
        append (T, num * sizeof (float32));
        for (int n = 0; n < num; n++) put<float32> (val[n], data(n), is_BE);
      }





      inline void MDS::append_float64 (const Tag& T, const float64* val, int num)
      {
        append (T, num * sizeof (float64));
        for (int n = 0; n < num; n++) put<float64> (val[n], data(n), is_BE);
      }



    }
  }
}

#endif



