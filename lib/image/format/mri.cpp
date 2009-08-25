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


#include "file/config.h"
#include "file/path.h"
#include "image/misc.h"
#include "file/mmap.h"
#include "image/format/list.h"
#include "image/header.h"
#include "get_set.h"

/*
 MRI format:
     Magic number:              MRI#         (4 bytes)
     Byte order specifier:    uint16_t = 1     (2 bytes)
     ...
     Elements:
       ID specifier:            uint32_t       (4 bytes)
       size:                    uint32_t       (4 bytes)
       contents:              unspecified  ('size' bytes)
     ...

*/

#define MRI_DATA       0x01
#define MRI_DIMENSIONS 0x02
#define MRI_ORDER      0x03
#define MRI_VOXELSIZE  0x04
#define MRI_COMMENT    0x05
#define MRI_TRANSFORM  0x06
#define MRI_DWSCHEME   0x07

namespace MR {
  namespace Image {
    namespace Format {

      namespace {

        const char* FormatMRI = "MRTools (legacy format)";

        inline size_t char2order (char item, bool& forward)
        {
          switch (item) {
            case 'L': forward = true;  return (0);
            case 'R': forward = false; return (0);
            case 'P': forward = true;  return (1);
            case 'A': forward = false; return (1);
            case 'I': forward = true;  return (2);
            case 'S': forward = false; return (2);
            case 'B': forward = true;  return (3);
            case 'E': forward = false; return (3);
          }
          return (Axis::undefined);
        }


        inline char order2char (size_t axis, bool forward)
        {
          switch (axis) {
            case 0: if (forward) return ('L'); else return ('R');
            case 1: if (forward) return ('P'); else return ('A');
            case 2: if (forward) return ('I'); else return ('S');
            case 3: if (forward) return ('B'); else return ('E');
          }
          return ('\0');
        }


        inline uint type (const uint8_t* pos, bool is_BE) { return (get<uint32_t> (pos, is_BE)); }
        inline size_t size (const uint8_t* pos, bool is_BE) { return (get<uint32_t> (pos + sizeof (uint32_t), is_BE)); }
        inline uint8_t* data (uint8_t* pos)                 { return (pos + 2*sizeof (uint32_t)); }
        inline const uint8_t* data (const uint8_t* pos)     { return (pos + 2*sizeof (uint32_t)); }

        inline uint8_t* next (uint8_t* current_pos, bool is_BE) 
        {
          return (current_pos + 2*sizeof (uint32_t) + size (current_pos, is_BE));
        }
        inline const uint8_t* next (const uint8_t* current_pos, bool is_BE) 
        {
          return (current_pos + 2*sizeof (uint32_t) + size (current_pos, is_BE));
        }
        inline void write (uint8_t* pos, uint32_t Type, uint32_t Size, bool is_BE)
        {
          put<uint32_t> (Type, pos, is_BE);
          put<uint32_t> (Size, pos + sizeof (uint32_t), is_BE);
        }

      }








      bool MRI::read (Mapper& dmap, Header& H) const
      {
        if (!Path::has_suffix (H.name, ".mri")) return (false);

        H.format = FormatMRI;

        File::MMap fmap (H.name);
        fmap.map();

        if (memcmp ((uint8_t*) fmap.address(), "MRI#", 4)) 
          throw Exception ("file \"" + H.name + "\" is not in MRI format (unrecognised magic number)");

        bool is_BE = false;
        if (get<uint16_t> ((uint8_t*) fmap.address() + sizeof (int32_t), is_BE) == 0x0100U) is_BE = true;
        else if (get<uint16_t> ((uint8_t*) fmap.address() + sizeof (uint32_t), is_BE) != 0x0001U) 
          throw Exception ("MRI file \"" + H.name + "\" is badly formed (invalid byte order specifier)");

        H.axes.resize (4);

        size_t data_offset = 0;
        char* c;
        Math::Matrix<float> M (4,4);
        const uint8_t* current = (uint8_t*) fmap.address() + sizeof (int32_t) + sizeof (uint16_t);
        const uint8_t* last = (uint8_t*) fmap.address() + fmap.size() - 2*sizeof (uint32_t);

        while (current <= last) {
          switch (type (current, is_BE)) {
            case MRI_DATA:
              H.data_type = (((const char*) data (current)) - 4)[0];
              data_offset = current + 5 - (uint8_t*) fmap.address();
              fmap.unmap();
              break;
            case MRI_DIMENSIONS:
              H.axes[0].dim = get<uint32_t> (data (current), is_BE);
              H.axes[1].dim = get<uint32_t> (data (current) + sizeof (uint32_t), is_BE);
              H.axes[2].dim = get<uint32_t> (data (current) + 2*sizeof (uint32_t), is_BE);
              H.axes[3].dim = get<uint32_t> (data (current) + 3*sizeof (uint32_t), is_BE);
              break;
            case MRI_ORDER:
              c = (char*) data (current);
              for (size_t n = 0; n < 4; n++) {
                bool forward = true;
                uint ax = char2order (c[n], forward);
                H.axes[ax].order = n;
                H.axes[ax].forward = forward;
              }
              break;
            case MRI_VOXELSIZE:
              H.axes[0].vox = get<float32> (data (current), is_BE);
              H.axes[1].vox = get<float32> (data (current) + sizeof (float32), is_BE);
              H.axes[2].vox = get<float32> (data (current) + 2*sizeof (float32), is_BE);
              break;
            case MRI_COMMENT:
              H.comments.push_back (std::string ((const char*) data (current), size (current, is_BE)));
              break;
            case MRI_TRANSFORM:
              for (size_t i = 0; i < 4; i++)
                for (size_t j = 0; j < 4; j++)
                  M(i,j) = get<float32> (data (current) + ( i*4 + j )*sizeof (float32), is_BE);
              H.transform_matrix = M;
              break;
            case MRI_DWSCHEME:
              H.DW_scheme.allocate (size (current, is_BE)/(4*sizeof (float32)), 4);
              for (size_t i = 0; i < H.DW_scheme.rows(); i++)
                for (size_t j = 0; j < 4; j++)
                  H.DW_scheme(i,j) = get<float32> (data (current) + ( i*4 + j )*sizeof (float32), is_BE);
              break;
            default:
              error ("unknown header entity (" + str (type (current, is_BE)) 
                  + ", offset " + str (current - (uint8_t*) fmap.address()) 
                  + ") in image \"" + H.name + "\" - ignored");
              break;
          }
          if (data_offset) break;
          current = next (current, is_BE);
        }


        if (!data_offset) throw Exception ("no data field found in MRI image \"" + H.name + "\"");


        if (!H.axes[0].desc.size()) H.axes[0].desc = Axis::left_to_right;
        if (!H.axes[0].units.size()) H.axes[0].units = Axis::millimeters;
        if (H.axes.size() > 1) {
          if (!H.axes[1].desc.size()) H.axes[1].desc = Axis::posterior_to_anterior;
          if (!H.axes[1].units.size()) H.axes[1].units = Axis::millimeters;
          if (H.axes.size() > 2) {
            if (!H.axes[2].desc.size()) H.axes[2].desc = Axis::inferior_to_superior;
            if (!H.axes[2].units.size()) H.axes[2].units = Axis::millimeters;
          }
        }

        dmap.add (fmap, data_offset);

        return (true);
      }





      bool MRI::check (Header& H, int num_axes) const
      {
        if (!Path::has_suffix (H.name, ".mri")) return (false);
        if ((int) H.axes.ndim() > num_axes && num_axes != 4) throw Exception ("MRTools format can only support 4 dimensions");

        H.format = FormatMRI;

        H.axes.resize (num_axes);

        if (H.axes[0].desc.empty()) H.axes[0].desc= Axis::left_to_right;
        if (H.axes[0].units.empty()) H.axes[0].units = Axis::millimeters;
        if (H.axes.size() > 1) {
          if (H.axes[1].desc.empty()) H.axes[1].desc = Axis::posterior_to_anterior;
          if (H.axes[1].units.empty()) H.axes[1].units = Axis::millimeters;
          if (H.axes.size() > 2) {
            if (H.axes[2].desc.empty()) H.axes[2].desc = Axis::inferior_to_superior;
            if (H.axes[2].units.empty()) H.axes[2].units = Axis::millimeters;
          }
        }

        return (true);
      }







      void MRI::create (Mapper& dmap, const Header& H) const
      {
        File::MMap fmap (H.name, 65536, "mri");
        fmap.map();

#ifdef BYTE_ORDER_BIG_ENDIAN
        bool is_BE = true;
#else
        bool is_BE = false;
#endif

        memcpy ((uint8_t*) fmap.address(), "MRI#", 4);
        put<uint16_t> (0x01U, (uint8_t*) fmap.address() + sizeof (uint32_t), is_BE);

        uint8_t* current = (uint8_t*) fmap.address() + sizeof (uint32_t) + sizeof (uint16_t);

        write (current, MRI_DIMENSIONS, 4*sizeof (uint32_t), is_BE);
        put<uint32_t> (H.axes[0].dim, data (current), is_BE);
        put<uint32_t> (( H.axes.size() > 1 ? H.axes[1].dim : 1 ), data (current) + sizeof (uint32_t), is_BE);
        put<uint32_t> (( H.axes.size() > 2 ? H.axes[2].dim : 1 ), data (current) + 2*sizeof (uint32_t), is_BE);
        put<uint32_t> (( H.axes.size() > 3 ? H.axes[3].dim : 1 ), data (current) + 3*sizeof (uint32_t), is_BE);

        current = next (current, is_BE);
        write (current, MRI_ORDER, 4*sizeof (uint8_t), is_BE);
        size_t n;
        for (n = 0; n < H.axes.size(); n++) 
          ((char*) data (current))[H.axes[n].order] = order2char (n, H.axes[n].forward);
        for (; n < 4; n++) ((char*) data (current))[n] = order2char (n, true);

        current = next (current, is_BE);
        write (current, MRI_VOXELSIZE, 3*sizeof (float32), is_BE);
        put<float32> (H.axes[0].vox, data (current), is_BE);
        put<float32> (( H.axes.size() > 1 ? H.axes[1].vox : 2.0 ), data (current)+sizeof (float32), is_BE);
        put<float32> (( H.axes.size() > 2 ? H.axes[2].vox : 2.0 ), data (current)+2*sizeof (float32), is_BE);

        for (size_t n = 0; n < H.comments.size(); n++) {
          size_t l = H.comments[n].size();
          if (l) {
            current = next (current, is_BE);
            write (current, MRI_COMMENT, l, is_BE);
            memcpy (data (current), H.comments[n].c_str(), l);
          }
        }

        if (H.transform_matrix.is_set()) {
          current = next (current, is_BE);
          write (current, MRI_TRANSFORM, 16*sizeof (float32), is_BE);
          for (size_t i = 0; i < 4; i++)
            for (size_t j = 0; j < 4; j++)
              put<float32> (H.transform_matrix(i,j), data (current) + ( i*4 + j )*sizeof (float32), is_BE);
        }

        if (H.DW_scheme.is_set()) {
          current = next (current, is_BE);
          write (current, MRI_DWSCHEME, 4*H.DW_scheme.rows()*sizeof (float32), is_BE);
          for (uint i = 0; i < H.DW_scheme.rows(); i++)
            for (uint j = 0; j < 4; j++)
              put<float32> (H.DW_scheme(i,j), data (current) + ( i*4 + j )*sizeof (float32), is_BE);
        }

        current = next (current, is_BE);
        write (current, MRI_DATA, 0, is_BE);
        ((char*) current)[4] = H.data_type();

        size_t data_offset = current + 5 - (uint8_t*) fmap.address();
        fmap.resize (data_offset + memory_footprint(H.data_type, voxel_count (H.axes)));
        dmap.add (fmap, data_offset);
      }

    }
  }
}



