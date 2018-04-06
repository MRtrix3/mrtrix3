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


#include "file/config.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mmap.h"
#include "formats/list.h"
#include "header.h"
#include "image_io/default.h"
#include "raw.h"

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

namespace MR
{
  namespace Formats
  {

    namespace
    {

      inline size_t char2order (char item, bool& forward)
      {
        switch (item) {
          case 'L':
            forward = true;
            return (0);
          case 'R':
            forward = false;
            return (0);
          case 'P':
            forward = true;
            return (1);
          case 'A':
            forward = false;
            return (1);
          case 'I':
            forward = true;
            return (2);
          case 'S':
            forward = false;
            return (2);
          case 'B':
            forward = true;
            return (3);
          case 'E':
            forward = false;
            return (3);
        }
        return (std::numeric_limits<size_t>::max());
      }


      inline char order2char (size_t axis, bool forward)
      {
        switch (axis) {
          case 0:
            if (forward) return ('L');
            else return ('R');
          case 1:
            if (forward) return ('P');
            else return ('A');
          case 2:
            if (forward) return ('I');
            else return ('S');
          case 3:
            if (forward) return ('B');
            else return ('E');
        }
        return ('\0');
      }


      inline size_t type (const uint8_t* pos, bool is_BE)
      {
        return (Raw::fetch_<uint32_t> (pos, is_BE));
      }
      inline size_t size (const uint8_t* pos, bool is_BE)
      {
        return (Raw::fetch_<uint32_t> (pos + sizeof (uint32_t), is_BE));
      }
      inline const uint8_t* data (const uint8_t* pos)
      {
        return (pos + 2*sizeof (uint32_t));
      }

      inline const uint8_t* next (const uint8_t* current_pos, bool is_BE)
      {
        return (current_pos + 2*sizeof (uint32_t) + size (current_pos, is_BE));
      }

      inline void write_tag (std::ostream& out, uint32_t Type, uint32_t Size, bool is_BE)
      {
        Type = ByteOrder::swap<uint32_t> (Type, is_BE);
        out.write ( (const char*) &Type, sizeof (uint32_t));
        Size = ByteOrder::swap<uint32_t> (Size, is_BE);
        out.write ( (const char*) &Size, sizeof (uint32_t));
      }

      template <typename T> inline void write (std::ostream& out, T val, bool is_BE)
      {
        val = ByteOrder::swap<T> (val, is_BE);
        out.write ( (const char*) &val, sizeof (T));
      }

      // needed to get around changes in hard-coded enum types in datatype.h:
      DataType fetch_datatype (uint8_t c) {
        uint8_t d = c & 0x07U;
        uint8_t t = c & ~(0x07U);
        if (d >= 0x05U) ++d;
        return { uint8_t (d | t) };
      }

      uint8_t store_datatype (const DataType& dt) {
        uint8_t d = dt() & 0x07U;
        uint8_t t = dt() & ~(0x07U);
        if (d >= 0x05U) --d;
        return (d | t);
      }

    }








    std::unique_ptr<ImageIO::Base> MRI::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".mri"))
        return std::unique_ptr<ImageIO::Base>();

      File::MMap fmap (H.name());

      if (memcmp (fmap.address(), "MRI#", 4))
        throw Exception ("file \"" + H.name() + "\" is not in MRI format (unrecognised magic number)");

      bool is_BE = false;
      if (Raw::fetch_<uint16_t> (fmap.address() + sizeof (int32_t), is_BE) == 0x0100U) is_BE = true;
      else if (Raw::fetch_<uint16_t> (fmap.address() + sizeof (uint32_t), is_BE) != 0x0001U)
        throw Exception ("MRI file \"" + H.name() + "\" is badly formed (invalid byte order specifier)");

      H.ndim() = 4;

      size_t data_offset = 0;
      char* c;
      const uint8_t* current = fmap.address() + sizeof (int32_t) + sizeof (uint16_t);
      const uint8_t* last = fmap.address() + fmap.size() - 2*sizeof (uint32_t);

      while (current <= last) {
        switch (type (current, is_BE)) {
          case MRI_DATA:
            H.datatype() = fetch_datatype (data(current)[-4]);
            data_offset = current + 5 - (uint8_t*) fmap.address();
            break;
          case MRI_DIMENSIONS:
            H.size(0) = Raw::fetch_<uint32_t> (data (current), is_BE);
            H.size(1) = Raw::fetch_<uint32_t> (data (current) + sizeof (uint32_t), is_BE);
            H.size(2) = Raw::fetch_<uint32_t> (data (current) + 2*sizeof (uint32_t), is_BE);
            H.size(3) = Raw::fetch_<uint32_t> (data (current) + 3*sizeof (uint32_t), is_BE);
            break;
          case MRI_ORDER:
            c = (char*) data (current);
            for (size_t n = 0; n < 4; n++) {
              bool forward = true;
              size_t ax = char2order (c[n], forward);
              if (ax == std::numeric_limits<size_t>::max())
                throw Exception ("invalid order specifier in MRI image \"" + H.name() + "\"");
              H.stride(ax) = n+1;
              if (!forward)
                H.stride(ax) = -H.stride (ax);
            }
            break;
          case MRI_VOXELSIZE:
            H.spacing(0) = Raw::fetch_<float32> (data (current), is_BE);
            H.spacing(1) = Raw::fetch_<float32> (data (current) + sizeof (float32), is_BE);
            H.spacing(2) = Raw::fetch_<float32> (data (current) + 2*sizeof (float32), is_BE);
            break;
          case MRI_COMMENT:
            add_line (H.keyval()["comments"], std::string (reinterpret_cast<const char*> (data (current)), size (current, is_BE)));
            break;
          case MRI_TRANSFORM:
            for (size_t i = 0; i < 3; ++i)
              for (size_t j = 0; j < 4; ++j)
                H.transform() (i,j) = Raw::fetch_<float32> (data (current) + (i*4 + j) *sizeof (float32), is_BE);
            break;
          case MRI_DWSCHEME:
            {
              std::string dw_scheme;
              const size_t nrows = size (current, is_BE) / (4*sizeof (float32));
              for (size_t i = 0; i < nrows; ++i) 
                dw_scheme += 
                  str (Raw::fetch_<float32> (data (current) + (i*4 + 0) *sizeof (float32), is_BE)) + "," +
                  str (Raw::fetch_<float32> (data (current) + (i*4 + 1) *sizeof (float32), is_BE)) + "," +
                  str (Raw::fetch_<float32> (data (current) + (i*4 + 2) *sizeof (float32), is_BE)) + "," +
                  str (Raw::fetch_<float32> (data (current) + (i*4 + 3) *sizeof (float32), is_BE)) + "\n";
              H.keyval()["dw_scheme"] = dw_scheme;
            }
            break;
          default:
            WARN ("unknown header entity (" + str (type (current, is_BE))
                + ", offset " + str (current - fmap.address())
                + ") in image \"" + H.name() + "\" - ignored");
            break;
        }

        if (data_offset)
          break;

        current = next (current, is_BE);
      }


      if (!data_offset)
        throw Exception ("no data field found in MRI image \"" + H.name() + "\"");

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name(), data_offset));

      return io_handler;
    }





    bool MRI::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".mri"))
        return false;

      if (H.ndim() > num_axes && num_axes != 4)
        throw Exception ("MRTools format can only support 4 dimensions");

      H.ndim() = num_axes;

      return true;
    }







    std::unique_ptr<ImageIO::Base> MRI::create (Header& H) const
    {
      File::OFStream out (H.name());

#ifdef MRTRIX_BYTE_ORDER_BIG_ENDIAN
      bool is_BE = true;
#else
      bool is_BE = false;
#endif

      out.write ("MRI#", 4);
      write<uint16_t> (out, 0x01U, is_BE);

      write_tag (out, MRI_DIMENSIONS, 4*sizeof (uint32_t), is_BE);
      write<uint32_t> (out, H.size (0), is_BE);
      write<uint32_t> (out, (H.ndim() > 1 ? H.size (1) : 1), is_BE);
      write<uint32_t> (out, (H.ndim() > 2 ? H.size (2) : 1), is_BE);
      write<uint32_t> (out, (H.ndim() > 3 ? H.size (3) : 1), is_BE);

      write_tag (out, MRI_ORDER, 4*sizeof (uint8_t), is_BE);
      size_t n;
      char order[4];
      for (n = 0; n < H.ndim(); ++n)
        order[std::abs (H.stride (n))-1] = order2char (n, H.stride (n) >0);
      for (; n < 4; ++n)
        order[n] = order2char (n, true);
      out.write (order, 4);

      write_tag (out, MRI_VOXELSIZE, 3*sizeof (float32), is_BE);
      write<float> (out, H.spacing (0), is_BE);
      write<float> (out, (H.ndim() > 1 ? H.spacing (1) : 2.0f), is_BE);
      write<float> (out, (H.ndim() > 2 ? H.spacing (2) : 2.0f), is_BE);

      const auto comments = H.keyval().find ("comments");
      if (comments != H.keyval().end()) {
        for (const auto comment : split_lines (comments->second)) {
          size_t l = comment.size();
          if (l) {
            write_tag (out, MRI_COMMENT, l, is_BE);
            out.write (comment.c_str(), l);
          }
        }
      }

      write_tag (out, MRI_TRANSFORM, 16*sizeof (float32), is_BE);
      for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 4; ++j)
          write<float> (out, H.transform() (i,j), is_BE);
      write<float> (out, 0.0f, is_BE);
      write<float> (out, 0.0f, is_BE);
      write<float> (out, 0.0f, is_BE);
      write<float> (out, 1.0f, is_BE);

      const auto dw_scheme = H.keyval().find ("dw_scheme");
      if (dw_scheme != H.keyval().end()) {
        const auto rows = split_lines (dw_scheme->second);
        write_tag (out, MRI_DWSCHEME, 4*rows.size() *sizeof (float32), is_BE);
        for (const auto row : rows) 
          for (const auto val : parse_floats (row)) 
            write<float> (out, val, is_BE);
      }

      write_tag (out, MRI_DATA, 1, is_BE);
      out.put (store_datatype (H.datatype()));

      size_t data_offset = out.tellp();
      out.close();

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      File::resize (H.name(), data_offset + footprint(H));
      io_handler->files.push_back (File::Entry (H.name(), data_offset));

      return io_handler;
    }

  }
}



