/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 29/09/12.

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

#include "datatype.h"
#include "get_set.h"
#include "file/mgh_utils.h"
#include "image/header.h"
#include "image/stride.h"
#include "math/vector.h"

namespace MR
{
  namespace File
  {
    namespace MGH
    {


      size_t read_header (Image::Header& H, const mgh_header& MGHH, int64_t file_size)
      {

        bool is_BE = false;
        if (get<int32_t> (&MGHH.version, is_BE) != 1) {
          is_BE = true;
          if (get<int32_t> (&MGHH.version, is_BE) != 1)
            throw Exception ("image \"" + H.name() + "\" is not in MGH format (version != 1)");
        }

        const size_t ndim = (get<int32_t> (&MGHH.nframes, is_BE) > 1) ? 4 : 3;
        H.set_ndim (ndim);
        H.dim (0) = get<int32_t> (&MGHH.width, is_BE);
        H.dim (1) = get<int32_t> (&MGHH.height, is_BE);
        H.dim (2) = get<int32_t> (&MGHH.depth, is_BE);
        if (ndim == 4)
          H.dim (3) = get<int32_t> (&MGHH.nframes, is_BE);

        H.vox (0) = get<float> (&MGHH.spacing_x, is_BE);
        H.vox (1) = get<float> (&MGHH.spacing_y, is_BE);
        H.vox (2) = get<float> (&MGHH.spacing_z, is_BE);

        for (size_t i = 0; i != ndim; ++i)
          H.stride (i) = i + 1;

        DataType dtype;
        int32_t type = get<int32_t> (&MGHH.type, is_BE);
        switch (type) {
          case MGH_TYPE_UCHAR: dtype = DataType::UInt8;   break;
          case MGH_TYPE_SHORT: dtype = DataType::Int16;   break;
          case MGH_TYPE_INT:   dtype = DataType::Int32;   break;
          case MGH_TYPE_FLOAT: dtype = DataType::Float32; break;
          default: throw Exception ("unknown data type for MGH image \"" + H.name() + "\" (" + str (type) + ")");
        }
        if (dtype != DataType::UInt8) {
          if (is_BE)
            dtype.set_flag (DataType::BigEndian);
          else
            dtype.set_flag (DataType::LittleEndian);
        }
        H.datatype() = dtype;

        H.intensity_offset() = 0.0;
        H.intensity_scale() = 1.0;

        Math::Matrix<float>& M (H.transform());
        M.allocate (4,4);

        const int16_t RAS = get<int16_t> (&MGHH.goodRASFlag, is_BE);
        if (RAS) {

          M (0,0) = get <float> (&MGHH.x_r, is_BE); M (0,1) = get <float> (&MGHH.y_r, is_BE); M (0,2) = get <float> (&MGHH.z_r, is_BE);
          M (1,0) = get <float> (&MGHH.x_a, is_BE); M (1,1) = get <float> (&MGHH.y_a, is_BE); M (1,2) = get <float> (&MGHH.z_a, is_BE);
          M (2,0) = get <float> (&MGHH.x_s, is_BE); M (2,1) = get <float> (&MGHH.y_s, is_BE); M (2,2) = get <float> (&MGHH.z_s, is_BE);

          M (0,3) = get <float> (&MGHH.c_r, is_BE);
          M (1,3) = get <float> (&MGHH.c_a, is_BE);
          M (2,3) = get <float> (&MGHH.c_s, is_BE);
          for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
              M (i,3) -= 0.5 * H.dim(j) * H.vox(j) * M(i,j);
          }

          M (3,0) = M (3,1) = M (3,2) = 0.0;
          M (3,3) = 1.0;

        } else {

          // Default transformation matrix, assumes coronal orientation
          M (0,0) = -1.0; M (0,1) =  0.0; M (0,2) =  0.0; M (0,3) = 0.0;
          M (0,0) =  0.0; M (0,1) =  0.0; M (0,2) = -1.0; M (0,3) = 0.0;
          M (0,0) =  0.0; M (0,1) = +1.0; M (0,2) =  0.0; M (0,3) = 0.0;
          M (0,0) =  0.0; M (0,1) =  0.0; M (0,2) =  0.0; M (0,3) = 1.0;

        }

        // Remaining header items appear AFTER the data
        // It's possible that these data may not even be there; need to make sure that we don't go over the file size
        // This will require an additional parameter sent to this function
        int64_t other_offset = MGH_DATA_OFFSET + Image::footprint (H);
        if (other_offset < file_size)
          read_other (H, *(mgh_other*)(&MGHH + other_offset), file_size - other_offset, is_BE);

        return MGH_DATA_OFFSET;

      }



      void read_other (Image::Header& H, const mgh_other& MGHO, const int64_t other_size, const bool is_BE) {

        // FIXME For some reason this is segfaulting
        // MGHO should lie within the memory-mapped region...
        return;

        if (get<float> (&MGHO.tr, is_BE))
          H.comments().push_back ("TR: "   + str (get<float> (&MGHO.tr, is_BE)) + "ms");
        if (get<float> (&MGHO.flip_angle, is_BE))
          H.comments().push_back ("Flip: " + str (get<float> (&MGHO.flip_angle, is_BE) * 180.0 / M_PI) + "deg");
        if (get<float> (&MGHO.te, is_BE))
          H.comments().push_back ("TE: "   + str (get<float> (&MGHO.te, is_BE)) + "ms");
        if (get<float> (&MGHO.ti, is_BE))
          H.comments().push_back ("TI: "   + str (get<float> (&MGHO.ti, is_BE)) + "ms");

        // Ignore FoV field

        const int64_t tags_offset = 5 * sizeof (float);
        if (other_size > tags_offset) {

          // It's memory-mapped, so should be able to use memcpy to do the initial grab
          size_t total_text_length = other_size - tags_offset;
          char* const tags = new char [total_text_length];
          memcpy (tags, &MGHO + tags_offset, total_text_length);

          // Extract and separate null-terminated strings
          size_t char_offset = 0;
          while (char_offset < total_text_length) {
            std::string line (tags + char_offset);
            if (line.size())
              H.comments().push_back ("Tag: " + line);
            char_offset += line.size() + 1;
          }

          delete[] tags;

        }

      }





      void write_header (mgh_header& MGHH, const Image::Header& H)
      {

        bool is_BE = H.datatype().is_big_endian();

        const size_t ndim = H.ndim();
        if (ndim > 4)
          throw Exception ("MGH file format does not support images of more than 4 dimensions");

        put<int32_t> (1, &MGHH.version, is_BE);
        put<int32_t> (H.dim (0), &MGHH.width, is_BE);
        put<int32_t> ((ndim > 1) ? H.dim (1) : 1, &MGHH.height, is_BE);
        put<int32_t> ((ndim > 2) ? H.dim (2) : 1, &MGHH.depth, is_BE);
        put<int32_t> ((ndim > 3) ? H.dim (3) : 1, &MGHH.nframes, is_BE);

        const DataType& dt = H.datatype();
        if (dt.is_complex())
          throw Exception ("MGH file format does not support complex types");
        if (dt.bits() == 64)
          throw Exception ("MGH file format does not support 64-bit floating-point format");
        if (dt.bits() == 1)
          throw Exception ("MGH file format does not support bit type");
        if (dt.bits() == 8 && !dt.is_signed())
          throw Exception ("MGH file format does not support signed 8-bit format; unsigned only");
        if (dt.bits() == 16 && !dt.is_signed())
          throw Exception ("MGH file format does not support unsigned 16-bit integer format; signed only");
        if (dt == DataType::UInt32 || dt == DataType::UInt32BE || dt == DataType::UInt32LE)
          throw Exception ("MGH file format does not support unsigned 32-bit integer format; signed only");
        int32_t type;
        switch (dt()) {
          case DataType::UInt8: type = MGH_TYPE_UCHAR; break;
          case DataType::Int16:   case DataType::Int16BE:   case DataType::Int16LE:   type = MGH_TYPE_SHORT; break;
          case DataType::Int32:   case DataType::Int32BE:   case DataType::Int32LE:   type = MGH_TYPE_INT;   break;
          case DataType::Float32: case DataType::Float32BE: case DataType::Float32LE: type = MGH_TYPE_FLOAT; break;
          default: throw Exception ("Error in MGH file format data type parsing");
        }
        put<int32_t> (type, &MGHH.type, is_BE);

        put<int32_t> (0, &MGHH.dof, is_BE);
        put<int16_t> (1, &MGHH.goodRASFlag, is_BE);
        put<float> (H.vox (0), &MGHH.spacing_x, is_BE);
        put<float> (H.vox (1), &MGHH.spacing_y, is_BE);
        put<float> (H.vox (2), &MGHH.spacing_z, is_BE);

        const Math::Matrix<float>& M (H.transform());
        put<float> (M (0,0), &MGHH.x_r, is_BE); put<float> (M (0,1), &MGHH.y_r, is_BE); put<float> (M (0,2), &MGHH.z_r, is_BE);
        put<float> (M (1,0), &MGHH.x_a, is_BE); put<float> (M (1,1), &MGHH.y_a, is_BE); put<float> (M (1,2), &MGHH.z_a, is_BE);
        put<float> (M (2,0), &MGHH.x_s, is_BE); put<float> (M (2,1), &MGHH.y_s, is_BE); put<float> (M (2,2), &MGHH.z_s, is_BE);

        for (size_t i = 0; i != 3; ++i) {
          float offset = M (i, 3);
          for (size_t j = 0; j != 3; ++j)
            offset += 0.5 * H.dim(j) * H.vox(j) * M (i,j);
          switch (i) {
            case 0: put<float> (offset, &MGHH.c_r, is_BE); break;
            case 1: put<float> (offset, &MGHH.c_a, is_BE); break;
            case 2: put<float> (offset, &MGHH.c_s, is_BE); break;
          }
        }

      }




      void write_other (mgh_other& MGHO, const Image::Header& H)
      {

        bool is_BE = H.datatype().is_big_endian();

        for (std::vector<std::string>::const_iterator i = H.comments().begin(); i != H.comments().end(); ++i) {
          const std::string key = i->substr (0, i->find_first_of (':'));
          if (key == "TR")
            put<float> (to<float> (i->substr (3)), &MGHO.tr, is_BE);
          else if (key == "Flip")
            put<float> (to<float> (i->substr (5)), &MGHO.flip_angle, is_BE);
          else if (key == "TE")
            put<float> (to<float> (i->substr (3)), &MGHO.te, is_BE);
          else if (key == "TI")
            put<float> (to<float> (i->substr (3)), &MGHO.ti, is_BE);
          else
            MGHO.tags.push_back (*i);
        }
        put<float> (0.0, &MGHO.fov, is_BE);

      }




      void write_other_to_file (const std::string& path, const mgh_other& MGHO)
      {
        std::ofstream out (path.c_str(), std::ios_base::app);
        out.write ((char*) &MGHO, 5 * sizeof (float));
        for (std::vector<std::string>::const_iterator i = MGHO.tags.begin(); i != MGHO.tags.end(); ++i)
          out.write (i->c_str(), i->size() + 1);
        out.close();
      }




    }
  }
}


