#include "image/stride.h"
#include "datatype.h"
#include "get_set.h"
#include "file/mgh_utils.h"
#include "image/header.h"
#include "math/vector.h"

namespace MR
{
  namespace File
  {
    namespace MGH
    {


      bool read_header (Image::Header& H, const mgh_header& MGHH)
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

        return is_BE;

      }



      void read_other (Image::Header& H, const mgh_other& MGHO, const bool is_BE) {

        if (get<float> (&MGHO.tr, is_BE) != 0.0f)
          H.comments().push_back ("TR: "   + str (get<float> (&MGHO.tr, is_BE)) + "ms");
        if (get<float> (&MGHO.flip_angle, is_BE) != 0.0f)
          H.comments().push_back ("Flip: " + str (get<float> (&MGHO.flip_angle, is_BE) * 180.0 / M_PI) + "deg");
        if (get<float> (&MGHO.te, is_BE) != 0.0f)
          H.comments().push_back ("TE: "   + str (get<float> (&MGHO.te, is_BE)) + "ms");
        if (get<float> (&MGHO.ti, is_BE) != 0.0f)
          H.comments().push_back ("TI: "   + str (get<float> (&MGHO.ti, is_BE)) + "ms");

        // Ignore FoV field

        for (std::vector<std::string>::const_iterator i = MGHO.tags.begin(); i != MGHO.tags.end(); ++i)
          H.comments().push_back (*i);

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
        if ((dt() & DataType::Type) == DataType::Bit)
          throw Exception ("MGH file format does not support bit type");
        if (((dt() & DataType::Type) == DataType::UInt8) && dt.is_signed())
          throw Exception ("MGH file format does not support signed 8-bit integers; unsigned only");
        if (((dt() & DataType::Type) == DataType::UInt16) && !dt.is_signed())
          throw Exception ("MGH file format does not support unsigned 16-bit integers; signed only");
        if (((dt() & DataType::Type) == DataType::UInt32) && !dt.is_signed())
          throw Exception ("MGH file format does not support unsigned 32-bit integers; signed only");
        if ((dt() & DataType::Type) == DataType::UInt64)
          throw Exception ("MGH file format does not support 64-bit integer data");
        if ((dt() & DataType::Type) == DataType::Float64)
          throw Exception ("MGH file format does not support 64-bit floating-point data");
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


